edata = load("exponentialRandoms");
pdata = load("paretoRandoms");

hist(edata);
hist(pdata);

hist(edata(1:1000), 200);
title('Exponential Distribution with mean 300', 'fontsize', 20);
xlabel('Bins', 'fontsize', 20);
ylabel('Number of values in a bin', 'fontsize', 20);
axis([0, 10000, 0, 1000]);
print('exponential_hist.ps');
print('exponential_hist.png');

hist(pdata(1:1000), 200);
title('Bounded Pareto Distribution with mean 300, upper bound 10e5', 'fontsize', 20);
xlabel('Bins', 'fontsize', 20);
ylabel('Number of values in a bin', 'fontsize', 20);
axis([0, 10000, 0, 1000]);
print('pareto_hist.ps');
print('pareto_hist.png');

