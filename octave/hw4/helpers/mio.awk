BEGIN {start = 0;
       end = 0;
}
// {
        a = $1;
        b = $2;
	if(start == 0)
	{	start = a;
		end = b;
	}
	else 
	end = b;
}
END {
	print end-start;
}
