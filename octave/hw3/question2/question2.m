clear all; close all;

stat = simpleServer(10000, 50);

plot(stat.eventTime, stat.arrivalTask1-stat.requestTask1);
hold on
plot(stat.eventTime, stat.requestTask2-stat.requestTask2, 'r');
xlabel('Time');
ylabel('Number of Tasks in Queue');
title('Number of Tasks in Queue vs Time(ms)');
legend('Task 1', 'Task 2');
print -f1 -r600 -depsc2 question2.eps;

