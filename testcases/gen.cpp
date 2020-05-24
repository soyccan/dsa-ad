#include <iostream>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <array>
#include <vector>
#include <algorithm>
#include <set>
#include <tuple>

using std::ios;
using std::cerr;
using std::vector;
using std::unordered_map;
using std::set;
using std::array;
using std::tuple;
using std::endl;
using std::function;
using std::string;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::max;
using std::min;
using std::get;
using std::make_tuple;
using std::reverse;
using std::to_string;

#define skip(ss) { string s; getline(ss, s, '\t'); }

typedef int Timestamp;
typedef array<string, 5> Prop;
typedef array<string, 3> Upt;
typedef array<int, 2> TimeSale;
typedef tuple<Timestamp, double> TimeAvg;
typedef tuple<string, int> Counter;

struct sortByProductAndTimestamp {
	bool operator() (const Prop &a, const Prop &b) const {
		return a[0] < b[0] || (a[0] == b[0] && a[1] < b[1]);
	}
};

bool sortByCount(const Counter &a, const Counter &b) {
	return get<1>(a) > get<1>(b);
}

bool sortByTimestampInv(const TimeSale &a, const TimeSale &b) {
	return a[0] > b[0];
}

bool compTimestamp(const TimeAvg &ta, const Timestamp &t) {
	return get<0>(ta) < t;
}

typedef string User;
typedef string Product;
typedef set<Prop, sortByProductAndTimestamp> Props;
typedef set<User> UserSet;
typedef vector<User> Users;

unordered_map<string, int> upt2sale;
vector<Upt> upts;
unordered_map<User, Props> user2props;
unordered_map<Product, UserSet> product2users;
UserSet userset, users_with_same_props;
Users users;
unordered_map<User, vector<TimeSale> > user2timesales;
TimeAvg *time_avgs;
int *user_range;
vector<Counter> user_numProps;
set<Product> products;
vector<Counter> product_numUsers;


#define STARS "********************\n"

void get_(stringstream &ss, ofstream &fout) {
	string u, p, t;
	ss >> u >> p >> t;
	fout << (upt2sale[u + p + t] - 1) / 999 << endl;
}

void purchased(stringstream &ss, ofstream &fout) {
	string u;
	ss >> u;
	char sep = ' ';
	for (auto &[product, timestamp, price, age_group, gender] : user2props[u])
		fout << product << sep << timestamp << sep << price << sep << age_group << sep << gender << endl;
}

void clicked(stringstream &ss, ofstream &fout) {
	string p1, p2;
	ss >> p1 >> p2;
	UserSet &users1 = product2users[p1], &users2 = product2users[p2];
	vector<string> both(users1.size() + users2.size());
	auto end = set_intersection(users1.begin(), users1.end(), users2.begin(), users2.end(), both.begin());
	both.resize(end - both.begin());
	for (auto user : both)
		fout << user << endl;
}

void profit(stringstream &ss, ofstream &fout) {
	Timestamp t;
	double theta;
	ss >> t >> theta;
	int start = 0, end, cnt = 0;
	for (size_t i = 0; i < users.size(); i++) {
		end = user_range[i];
		auto it = lower_bound(time_avgs + start, time_avgs + end, t, compTimestamp);
		if (get<1>(*it) >= theta) {
			fout << users[i] << endl;
			if (++cnt == 10)
				break;
		}
		start = end;
	}
}

void parse(string line, int &sale, string &timestamp, string &price, string &age_group, string &gender, string &product, string &user) {
	stringstream sin(line);

	string sale_str;
	getline(sin, sale_str, '\t');
	sale = sale_str[0] - '0';
	skip(sin);
	skip(sin);
	getline(sin, timestamp, '\t');
	skip(sin);
	getline(sin, price, '\t');
	getline(sin, age_group, '\t');
	skip(sin);
	skip(sin);
	getline(sin, gender, '\t');
	skip(sin);
	for (int i = 0; i < 7; i++)
		skip(sin);
	skip(sin);
	getline(sin, product, '\t');
	skip(sin);
	skip(sin);
	getline(sin, user, '\t');
}

int main(int argc, char* argv[]) {
	srand(1);
	ios::sync_with_stdio(false);
	if (argc != 4) {
		cerr << "Usage: ./gen /path/to/data/file <#lines from data> <#commands>" << endl;
		exit(1);
	}
	string data_path(argv[1]), line;
	int num_reads = atoi(argv[2]), num_cmds = atoi(argv[3]);
	ifstream fin(data_path);
	int sale;
	string timestamp, price, age_group, gender, product, user;
	Timestamp max_timestamp = 0;
	int num_lines = 0;
	for (int i = 0; i < num_reads && getline(fin, line); i++) {
		parse(line, sale, timestamp, price, age_group, gender, product, user);

		if (user != "-1" && product != "-1" && timestamp != "-1") {
			string upt = user + product + timestamp;
			upt2sale[upt] += (sale ? 1000 : 1);
			upts.push_back({user, product, timestamp});
		}
		if (user != "-1" && sale == 1) {
			auto prop = Prop({product, timestamp, price, age_group, gender});
			if (user2props[user].count(prop))
				users_with_same_props.insert(user);
			else
				user2props[user].insert(prop);
		}
		if (product != "-1") {
			product2users[product].insert(user);
			products.insert(product);
		}
		userset.insert(user);
        int t = stoi(timestamp);
		user2timesales[user].push_back({t, sale});
		max_timestamp = max(max_timestamp, t);
		num_lines++;
	}
	fin.close();

	users.assign(userset.begin(), userset.end());
	time_avgs = new TimeAvg[num_lines + users.size()];
	user_range = new int[users.size()];
	int start, end = 0;
	for (size_t i = 0; i < users.size(); i++) {
		start = end;
		auto &timesales = user2timesales[users[i]];
		sort(timesales.begin(), timesales.end(), sortByTimestampInv);
		double clicks = 0, sales = 0;
		time_avgs[end++] = make_tuple(max_timestamp + 1, 0); // for timestamps larger than the user's
		for (auto &[time, sale] : timesales) {
			clicks++;
			sales += sale;
			time_avgs[end++] = make_tuple(time, sales / clicks);
		}
		reverse(time_avgs + start, time_avgs + end);
		user_range[i] = end;
	}
	
	unordered_map< string, function<void(stringstream&, ofstream&)> > run = {
		{"get", get_},
		{"purchased", purchased},
		{"clicked", clicked},
		{"profit", profit}
	};

	string in_name = "testing" + to_string(num_cmds) + ".in";
	string out_name = "testing" + to_string(num_cmds) + ".out";
	ofstream test_in(in_name);
	ofstream test_out(out_name);
	double p_get = 0.1, p_purchased = 0.1, p_clicked = 0.2, p_profit = 0.6;
	int cnt = 0;
	while (cnt < int(num_cmds * p_get)) {
		auto &[u, p, t] = upts[rand() % upts.size()];
		string upt = u + p + t;
		if (!(upt2sale[upt] % 1000 && upt2sale[upt] / 1000)) { // make sure not both sale == 0 and sale == 1 occurred
			stringstream ss;
			ss << u << ' ' << p << ' ' << t;
			test_in << "get " + ss.str() << endl;

			test_out << STARS;
			get_(ss, test_out);
			test_out << STARS;
			cnt++;
		}
	}

	for (auto user : users)
		user_numProps.push_back(make_tuple(user, user2props[user].size()));
	sort(user_numProps.begin(), user_numProps.end(), sortByCount);
	int u_cnt = 0;
	cnt = 0;
	while (cnt < int(num_cmds * p_purchased)) {
		string u;
		if (cnt < 10)
			u = users[rand() % users.size()];
		else
			u = get<0>(user_numProps[u_cnt++]);
		if (!users_with_same_props.count(u)) {
			stringstream ss;
			ss << u;
			test_in << "purchased " + ss.str() << endl;

			test_out << STARS;
			purchased(ss, test_out);
			test_out << STARS;
			cnt++;
		}
	}

	for (auto product : products)
		product_numUsers.push_back(make_tuple(product, product2users[product].size()));
	sort(product_numUsers.begin(), product_numUsers.end(), sortByCount);
    int p1_cnt = 0, p2_cnt = 0, p_large = 64;
	cnt = 0;
	while (cnt < int(num_cmds * p_clicked)) {
        stringstream ss;
        if (++p2_cnt == p_large)
            p2_cnt = ++p1_cnt + 1;
        string p1 = get<0>(product_numUsers[p1_cnt]);
        string p2 = get<0>(product_numUsers[p2_cnt]);
        ss << p1 << ' ' << p2;
        test_in << "clicked " + ss.str() << endl;

        test_out << STARS;
        clicked(ss, test_out);
        test_out << STARS;
		cnt++;
	}

	cnt = 0;
    double thetas[] = {0.9, 0.5, 0.33, 0.66, 0.25, 0.2};
	while (cnt < int(num_cmds * p_profit)) {
        int i = rand() % (num_lines + users.size());
        auto [t, _] = time_avgs[i];
        if (t != max_timestamp + 1) {
            stringstream ss;
            double theta = thetas[cnt % 6];
            ss << t << ' ' << theta;
            test_in << "profit " + ss.str() << endl;

            test_out << STARS;
            profit(ss, test_out);
            test_out << STARS;
            cnt++;
        }
	}
    test_in << "quit" << endl;

	delete[] time_avgs;
	delete[] user_range;

	return 0;
}

