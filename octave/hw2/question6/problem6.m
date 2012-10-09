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
speed_avg = [];
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
		time_curr = sqrt((x_curr-x_prev)*(x_curr-x_prev) + (y_curr-y_prev)*(y_curr-y_prev)) / v_curr;
		
		t = t + time_curr;
		
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

prev = 1;
for i=1:60
	tmp = speed(prev:prev+waypoints(i)-1);
	len = length(tmp);
	r = ceil(len*rand(1, 1));
	
	s = 0;
	for j=1:r
		s = s + tmp(ceil(len*rand(1, 1)));
	end
	
	speed_avg = [speed_avg, s/r];
	prev = prev + waypoints(i);
end

hist(speed_avg); 
xlabel('Speed');
ylabel('Frequency of the seen speed');
title('Frequency Distribution of Average Speed of 30 mobiles at random samples');
print -r1600 -depsc2 'question6_hist_30'
print -r1600 -dpng 'question6_hist_30'

md = sort(speed_avg);
c = cumsum(md);

xl = [0];
yl = [0];
n = length(md);
for i=1:n
	xl = [xl, i/n];
	yl = [yl, c(i)/c(n)];
end

plot(xl, yl);
hold on 
plot(0:1/n:1, 0:1/n:1);

xlabel('Percentage');
ylabel('Total wealth');
title('Lorentz Curve');
print -r1600 -depsc2 'question6_hist_30_lorentz'
print -r1600 -dpng 'question6_hist_30_lorentz'