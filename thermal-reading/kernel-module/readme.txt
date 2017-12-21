to run use the following command sequence

make
sudo insmod read.ko
sudo rmmod read

to see the results open the kernel log (kern.log) available at:

/var/log/kern.log

the result will be at the line with the execution start time

Example:

Dec 21 16:46:44 leonardohorstmann-Inspiron-5537 kernel: [ 1553.285564] msr read 2285307904 
Dec 21 16:46:44 leonardohorstmann-Inspiron-5537 kernel: [ 1553.285565] msr read 40108032 
Dec 21 16:46:44 leonardohorstmann-Inspiron-5537 kernel: [ 1553.285565] msr read 45 
Dec 21 16:46:55 leonardohorstmann-Inspiron-5537 kernel: [ 1563.580182] Temperature read finalized

The tests were executed on a Intel i7-4500U
Depending on your processor version it may not work, especially on elder architecture models
