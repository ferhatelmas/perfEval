BEGIN {
	old = 0; 
	sum = 0;
	n = 0;
} 
// {
	new = $1 + $2/1000000; 
	if (old > 0) {
		sum = sum + new - old;
		n = n + 1;
	}
	old = new;
}
END {
	print nou " " sum/n;
}