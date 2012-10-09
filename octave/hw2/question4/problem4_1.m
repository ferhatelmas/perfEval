start_time = cputime;

N = 100;
l = 1000;
L = 1000;
v_min = 1;
v_max = 2;
T = 86400;

x = [];
y = [];
speed = [];
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
		v_curr = (v_min+(v_max-v_min)*rand(1, 1));
		
		t = t + sqrt((x_curr-x_prev)*(x_curr-x_prev) + (y_curr-y_prev)*(y_curr-y_prev)) / v_curr;
		
		x = [x, x_curr];
		y = [y, y_curr];
		speed = [speed, v_curr];
		
		x_prev = x_curr;
		y_prev = y_curr;
		j = j + 1;
	end
	
	waypoints = [waypoints, j];
end

elapsed_time = cputime - start_time;
elapsed_time

% histogram of the speeds for one mobile
hist(speed(1:waypoints(1)));
xlabel('Speed of the mobile');
ylabel('Frequency of the speed');
title('Speed distribution of one mobile');
print -r1600 -depsc2 'question4_one'
print -r1600 -dpng 'question4_one'

% histogram of the speeds for all mobiles
hist(speed);
xlabel('Speed of the mobile');
ylabel('Frequency of the speed');
title('Speed distribution of all mobiles');
print -r1600 -depsc2 'question4_all'
print -r1600 -dpng 'question4_all'