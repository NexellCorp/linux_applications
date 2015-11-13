#!/bin/sh

run_io()
{
	while true
	do
		dd if=/dev/urandom of=sample.txt bs=8k count=1 2>&1 | cat > /dev/null
		sync
		#rm sample.txt
		#sync
		#echo 3 > /proc/sys/vm/drop_caches
	done
}

run_io
