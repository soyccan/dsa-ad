for var in hws/*.zip
do
	rm -rf sandbox
	mkdir sandbox
	studentid=$(echo $var | sed 's/hws\///g' | sed 's/.zip//g')
	echo "Processing ${studentid}"
	if [ ! -f "outputs/${studentid}.log" ]; then
		exec 1>>$"outputs/${studentid}.log" 2>&1
		set -x
		echo $var
		cp $var sandbox/
		pwd
		cd sandbox
		unzip "${studentid}.zip"
		echo "***Makefile***"
		make &>> "../outputs/${studentid}.log"
		echo "***Run for first testcase***"
		# gen.cpp  testing10000.in  testing10000.out  testing1000.in  testing1000.out  testing100.in  testing100.out
		timeout 1h ./demo /tmp2/judging_dsahw2_htlin/ceiba_crawl/data/CriteoSearchData.100000 < ../testcases/testing100.in > $"../outputs/${studentid}.1.out"
		timeout 1h ./demo /tmp2/judging_dsahw2_htlin/ceiba_crawl/data/CriteoSearchData.1000000 < ../testcases/testing1000.in > $"../outputs/${studentid}.2.out"
		timeout 1h ./demo /tmp2/judging_dsahw2_htlin/ceiba_crawl/data/CriteoSearchData < ../testcases/testing10000.in > $"../outputs/${studentid}.3.out"
		cd ..
	fi
done
