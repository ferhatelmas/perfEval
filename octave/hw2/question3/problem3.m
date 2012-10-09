start_time = cputime;

N = 100;
l = 1000;
L = 1000;
v_min = 1;
v_max = 2;
T = 86400;

x = [];
y = [];
waypoints = [];
trajectory_usr_cnt = 10;

for i=1:N
	t = 0;
	j = 0;
	x_prev = 0;
	y_prev = 0;
	while(t < T)
		x_curr = L*rand(1, 1);
		y_curr = l*rand(1, 1);
		
		t = t + sqrt((x_curr-x_prev)*(x_curr-x_prev) + (y_curr-y_prev)*(y_curr-y_prev)) / (v_min+(v_max-v_min)*rand(1, 1));
		
		x = [x, x_curr];
		y = [y, y_curr];
		
		x_prev = x_curr;
		y_prev = y_curr;
		j = j + 1;
	end
	
	waypoints = [waypoints, j];
end

elapsed_time = cputime - start_time;
elapsed_time

% plot min, mean and max of waypoints
plot(waypoints);
xlabel('Users');
ylabel('Number of Waypoints');
title(strcat('Wayspoints Versus Users Min=', num2str(min(waypoints)), ' Mean=', num2str(mean(waypoints)), ' Max=', num2str(max(waypoints))));
print -r1600 -depsc2 'problem3_1'
print -r1600 -dpng 'problem3_1'

% trajectory of one user
plot(x(1:waypoints(1)), y(1:waypoints(1)));
xlabel('L');
ylabel('l');
title('Trajectory of one user');
print -r1600 -depsc2 'problem3_2_trajectory_one_user'
print -r1600 -dpng 'problem3_2_trajectory_one_user'

% waypoints of one user
plot(x(1:waypoints(1)), y(1:waypoints(1)), '.');
xlabel('L');
ylabel('l');
title('Waypoints of one user');
print -r1600 -depsc2 'problem3_2_waypoints_one_user'
print -r1600 -dpng 'problem3_2_waypoints_one_user'

% trajectory of 10 users
hold on
prev = 1;
for i=1:trajectory_usr_cnt
	plot(x(prev:prev+waypoints(i)), y(prev:prev+waypoints(i)));
	prev = prev + waypoints(i);
end
xlabel('L');
ylabel('l');
title('Trajectory of 10 users');
print -r1600 -depsc2 'problem3_2_trajectory_10_users'
print -r1600 -dpng 'problem3_2_trajectory_10_users'

% clear for the waypoints
clf;

% waypoints of 10 users
num_of_points = sum(waypoints(1:trajectory_usr_cnt));
plot(x(1:num_of_points), y(1:num_of_points), '.');
xlabel('L');
ylabel('l');
title('Waypoints of 10 users');
print -r1600 -depsc2 'problem3_2_waypoints_10_users'
print -r1600 -dpng 'problem3_2_waypoints_10_users'
