d = load('d.log');
t = load('t.log');

plot(d(:, 1), d(:, 2), 'rx', 'MarkerSize', 10);
hold on;
plot(t(:, 1), t(:, 2), 'b+', 'MarkerSize', 10);
legend('D value', 'T value', 'location', 'east');
legend('show');
xlabel('Number of Threads(4 per user)');
ylabel('D and T values');
title('Average Inter Departure Time and Average Number of Requests vs Number of Users');

print('dt.ps');
