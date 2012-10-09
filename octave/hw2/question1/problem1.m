for n=10:10:100
   for i=1:9
	x = randn(n, 1);
	hist(x);
	ylabel('Number of iid normal variables in each bucket');
   	xlabel('Buckets of the iid standard normal variables');
	title(strcat('Histogram of ', int2str(n), ' iid normal variables'));
    	print(strcat('n_', int2str(n), '_', int2str(i), '.eps'));
	clf;	
   end
end
