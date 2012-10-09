x = 50 + 10 * randn(500, 1);
qqplot(x);
ylabel('Quantiles of 500 random normal values');
title('QQ Plot of Normal(50, 10) versus Standard Normal');
print -r1600 -depsc2 '2_2_normal';
print -r1600 -dpng '2_2_normal';

x = trnd(2, 500, 1);
qqplot(x);
ylabel('Quantiles of 500 random student values');
title('QQ Plot of Student(2) versus Standard Normal');
print -r1600 -depsc2 '2_2_student_2';
print -r1600 -dpng '2_2_student_2';

x = trnd(50, 500, 1);
qqplot(x);
ylabel('Quantiles of 500 random student values');
title('QQ Plot of Student(50) versus Standard Normal');
print -r1600 -depsc2 '2_2_student_50';
print -r1600 -dpng '2_2_student_50';

x = exprnd(0.5, 500, 1);
qqplot(x);
ylabel('Quantiles of 500 random exponential values');
title('QQ Plot of Exponential(0.5) versus Standard Normal');
print -r1600 -depsc2 '2_2_exponential';
print -r1600 -dpng '2_2_exponential';