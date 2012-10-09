x = 0:100;
plot(x, normpdf(x, 50, 10));
xlabel('x');
ylabel('P(x)');	
title('Normal Distribution mu=50, variance=10');
print -r1600 -depsc2 '2_1_normal';
print -r1600 -dpng '2_1_normal';

x = -25:0.25:25;
plot(x, tpdf(x, 2));
xlabel('x');
ylabel('P(x)');	
title('Student(t) Distribution with n=2 degree of freedom')
print -r1600 -depsc2 '2_1_student_2';
print -r1600 -dpng '2_1_student_2';

plot(x, tpdf(x, 50));
xlabel('x');
ylabel('P(x)');	
title('Student(t) Distribution with n=50 degree of freedom');
print -r1600 -depsc2 '2_1_student_50';
print -r1600 -dpng '2_1_student_50';

x = 0:0.1:5;
plot(x, exppdf(x, 0.5));
xlabel('x');
ylabel('P(x)');	
title('Exponential Distribution with lambda=0.5');
print -r1600 -depsc2 '2_1_exponential';
print -r1600 -dpng '2_1_exponential';

