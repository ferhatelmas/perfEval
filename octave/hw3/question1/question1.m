clear all; close all;

stat = simpleServer(10000, 50);

plot(stat.eventTime, stat.arrivalTask1);
hold on
plot(stat.eventTime, stat.requestTask2, 'r');
xlabel('Time');
ylabel('Number of Requests and Services');
title('Number of Requests and Services vs Time(ms)');
legend('Arrivals', 'Services');
print -f1 -r600 -depsc2 question1.eps;

