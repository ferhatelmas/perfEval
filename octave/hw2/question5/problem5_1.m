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
speed_freq = zeros(T/10, 1);
speed_cnt = zeros(T/10, 1);
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
		
		% modify if to get samples from more users
		if(i == 1)
			if(floor((t+time_curr)/10) - floor(t/10) > 0 && t+time_curr < T) 
				for z=floor(t/10)+1:floor((t+time_curr)/10)
					speed_freq(z) = speed_freq(z) + v_curr;
					speed_cnt(z) = speed_cnt(z) + 1;
				end
			end
		end
		
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

hist(speed_freq./speed_cnt);

xlabel('Speed');
ylabel('Frequency of the speed seen at each 10s');
title('Frequency Distribution of the seen speed by one mobile');
print -r1600 -depsc2 'question5_speed_one'
print -r1600 -dpng 'question5_speed_one'
