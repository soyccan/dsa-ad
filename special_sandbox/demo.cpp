#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <string>
#include <cmath>
#include <vector>
#include <set>
#include <fstream>

const int limit = 1000000;

using namespace std;

class Product {
public:
	string id;
	mutable set<string> user_clicked;
	Product() {

	}
	~Product() {

	}
	Product(string product_id) {
		id = product_id;
	}
	bool operator< (const Product& rhs) {
		return (id < rhs.id);
	}
};

bool operator< (const Product& lhs, const Product& rhs) {
	return (lhs.id < rhs.id);
}

class Purchase {
public:
	string id;
	string timestamp;
	string price;
	string age_group;
	string gender;
	Purchase() {

	}
	~Purchase() {

	}
	Purchase(string product_id, string click_timestamp, string product_price, string product_age_group, string product_gender) {
		id = product_id;
		timestamp = click_timestamp;
		price = product_price;
		age_group = product_age_group;
		gender = product_gender;
	}
	Purchase(string product_id, string click_timestamp) {
		id = product_id;
		timestamp = click_timestamp;
	}
	void print() const {
		cout << id << " " << timestamp << " " << price << " " << age_group << " " << gender << endl;
	}
	bool operator< (const Purchase& rhs) {
		if (id < rhs.id)return true;
		if (id > rhs.id)return false;
		if (timestamp < rhs.timestamp)return true;
		return false;
	}
};

bool operator< (const Purchase& lhs, const Purchase& rhs) {
	if (lhs.id < rhs.id)return true;
	if (lhs.id > rhs.id)return false;
	if (lhs.timestamp < rhs.timestamp)return true;
	return false;
}

class Click {
public:
	string timestamp;
	bool sold;
	Click() {

	}
	~Click() {

	}
	Click(string click_timestamp, bool sale) {
		timestamp = click_timestamp;
		sold = sale;
	}
	bool operator< (const Click& rhs) {
		return (timestamp < rhs.timestamp);
	}
};

bool operator< (const Click& lhs, const Click& rhs) {
	return (lhs.timestamp < rhs.timestamp);
}

class User {
public:
	mutable set<Purchase> purchased;
	mutable set<Click> clicked;
	string id;
	User() {

	}
	~User() {

	}
	User(string user_id) {
		id = user_id;
	}
	bool operator< (const User& rhs) {
		return (id < rhs.id);
	}
};

bool operator< (const User& lhs, const User& rhs) {
	return (lhs.id < rhs.id);
}

void getUPT(string user_id, string product_id, string click_timestamp, set<User> All_User) {
	set<User>::iterator user_it;
	user_it = All_User.find(User(user_id));
	if (user_it == All_User.end()) {
		cout << "this data has not been read" << endl;
		return;
	}
	if (user_it->purchased.find(Purchase(product_id, click_timestamp)) == user_it->purchased.end()) {
		cout << 0;
	}
	else {
		cout << 1;
	}
}

void purchaseU(string user_id, set<User> All_User) {
	set<User>::iterator user_it;
	set<Purchase>::iterator purchase_it;
	user_it = All_User.find(User(user_id));
	if (user_it == All_User.end()) {
		cout << "this data has not been read" << endl;
		return;
	}
	purchase_it = user_it->purchased.begin();
	while (purchase_it != user_it->purchased.end()) {
		purchase_it->print();
		purchase_it++;
	}
}

void clickedP1P2(string product_id1, string product_id2, set<Product> All_Product) {
	set<Product>::iterator product_it1;
	set<Product>::iterator product_it2;
	set<string>::iterator user_1;
	set<string>::iterator user_2;
	product_it1 = All_Product.find(Product(product_id1));
	product_it2 = All_Product.find(Product(product_id2));
	if (product_it1 == All_Product.end()) {
		cout << "this data has not been read" << endl;
		return;
	}
	if (product_it2 == All_Product.end()) {
		cout << "this data has not been read" << endl;
		return;
	}
	user_1 = product_it1->user_clicked.begin();
	user_2 = product_it2->user_clicked.begin();
	while (user_1 != product_it1->user_clicked.end() && user_2 != product_it2->user_clicked.end()) {
		if (*user_1 == *user_2) {
			cout << *user_1 << endl;
		}
		else if (*user_1 < *user_2) {
			user_1++;
		}
		else {
			user_2++;
		}
	}
}

void profit(string click_timestamp, float theta, set<User> All_User) {
	set<User>::iterator user_it;
	set<Click>::iterator click_it;
	user_it = All_User.begin();
	int count = 0;
	float click_count = 0;
	float purchase_count = 0;
	while (count < 10 && user_it != All_User.end()) {
		click_count = 0;
		purchase_count = 0;
		click_it = user_it->clicked.begin();
		while (click_it != user_it->clicked.end()) {
			if (click_it->timestamp >= click_timestamp) {
				click_count += 1.0;
				if (click_it->sold == true) {
					purchase_count += 1.0;
				}
			}
			click_it++;
		}
		if (purchase_count / click_count > theta) {
			cout << user_it->id;
			count++;
		}
		user_it++;
	}
}

int main(int argc, char* argv[]) {
	set<User> All_User;
	set<User>::iterator user_it;
	set<Product> All_Product;
	set<Product>::iterator product_it;
	ifstream file(argv[1]);
	string line;
	string* data = new string[23];
	string delimiter = "\t";
	int count = 0;
	int i;
	int pos;
	while (getline(file, line)&& count < limit) {
		pos = 0;
		for (i = 0; i < 22; i++) {
			pos = line.find_first_of(delimiter);
			data[i] = line.substr(0, pos);
			line.erase(0, pos + delimiter.length());
		}
		data[22] = line;
		user_it = All_User.insert(User(data[22])).first;
		product_it = All_Product.insert(Product(data[19])).first;
		if (stoi(data[0]) == 1)
			user_it->purchased.insert(Purchase(data[19], data[3], data[5], data[6], data[9]));
		user_it->clicked.insert(Click(data[3], stoi(data[0])));
		product_it->user_clicked.insert(data[22]);
		count++;
	}
	string input;
	getline(cin, input);
	delimiter = " ";
	string* parameter = new string[4];
	int num_of_parameter = 0;
	while (input != "quit") {
		cout << "********************" << endl;
		pos = 0;
		pos = input.find_first_of(delimiter);
		parameter[0] = input.substr(0, pos);
		input.erase(0, pos + delimiter.length());
		if (parameter[0] == "get") {
			num_of_parameter = 3;
		}
		else if (parameter[0] == "purchased") {
			num_of_parameter = 1;
		}
		else {
			num_of_parameter = 2;
		}
		for (i = 1; i <= num_of_parameter; i++) {
			pos = input.find_first_of(delimiter);
			parameter[i] = input.substr(0, pos);
			input.erase(0, pos + delimiter.length());
		}
		if (parameter[0] == "get") {
			getUPT(parameter[1], parameter[2], parameter[3], All_User);
		}
		else if (parameter[0] == "purchased") {
			purchaseU(parameter[1], All_User);
		}
		else if (parameter[0] == "clicked") {
			clickedP1P2(parameter[1], parameter[2], All_Product);
		}
		else if (parameter[0] == "profit") {
			profit(parameter[1], stof(parameter[2]), All_User);
		}
		cout << "********************" << endl;
		getline(cin, input);
	}
}