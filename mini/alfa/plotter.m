% distribution and policy names
distributionNames = cellstr({'exponential', 'pareto'});
policyNames = cellstr({'RANDOM', 'ROUND_ROBIN', 'DYNAMIC', 'SIZE_BASED'});

% load data 
data1random = load('distributions/pareto/random/pareto_1.1_RANDOM');
data2random = load('distributions/pareto/random/pareto_1.2_RANDOM');
data3random = load('distributions/pareto/random/pareto_1.3_RANDOM');
data4random = load('distributions/pareto/random/pareto_1.4_RANDOM');
data5random = load('distributions/pareto/random/pareto_1.5_RANDOM');
data6random = load('distributions/pareto/random/pareto_1.6_RANDOM');
data7random = load('distributions/pareto/random/pareto_1.7_RANDOM');
data8random = load('distributions/pareto/random/pareto_1.8_RANDOM');
data9random = load('distributions/pareto/random/pareto_1.9_RANDOM');

data1round = load('distributions/pareto/round/pareto_1.1_ROUND_ROBIN');
data2round = load('distributions/pareto/round/pareto_1.2_ROUND_ROBIN');
data3round = load('distributions/pareto/round/pareto_1.3_ROUND_ROBIN');
data4round = load('distributions/pareto/round/pareto_1.4_ROUND_ROBIN');
data5round = load('distributions/pareto/round/pareto_1.5_ROUND_ROBIN');
data6round = load('distributions/pareto/round/pareto_1.6_ROUND_ROBIN');
data7round = load('distributions/pareto/round/pareto_1.7_ROUND_ROBIN');
data8round = load('distributions/pareto/round/pareto_1.8_ROUND_ROBIN');
data9round = load('distributions/pareto/round/pareto_1.9_ROUND_ROBIN');

data1dynamic = load('distributions/pareto/dynamic/pareto_1.1_DYNAMIC');
data2dynamic = load('distributions/pareto/dynamic/pareto_1.2_DYNAMIC');
data3dynamic = load('distributions/pareto/dynamic/pareto_1.3_DYNAMIC');
data4dynamic = load('distributions/pareto/dynamic/pareto_1.4_DYNAMIC');
data5dynamic = load('distributions/pareto/dynamic/pareto_1.5_DYNAMIC');
data6dynamic = load('distributions/pareto/dynamic/pareto_1.6_DYNAMIC');
data7dynamic = load('distributions/pareto/dynamic/pareto_1.7_DYNAMIC');
data8dynamic = load('distributions/pareto/dynamic/pareto_1.8_DYNAMIC');
data9dynamic = load('distributions/pareto/dynamic/pareto_1.9_DYNAMIC');

data1size = load('distributions/pareto/size/pareto_1.1_SIZE_BASED');
data2size = load('distributions/pareto/size/pareto_1.2_SIZE_BASED');
data3size = load('distributions/pareto/size/pareto_1.3_SIZE_BASED');
data4size = load('distributions/pareto/size/pareto_1.4_SIZE_BASED');
data5size = load('distributions/pareto/size/pareto_1.5_SIZE_BASED');
data6size = load('distributions/pareto/size/pareto_1.6_SIZE_BASED');
data7size = load('distributions/pareto/size/pareto_1.7_SIZE_BASED');
data8size = load('distributions/pareto/size/pareto_1.8_SIZE_BASED');
data9size = load('distributions/pareto/size/pareto_1.9_SIZE_BASED');

dataExpAql           = load('distributions/exponential/exponential_aql');
dataParetoRandomAql  = load('distributions/pareto/random/pareto_random_aql');
dataParetoRoundAql   = load('distributions/pareto/round/pareto_round_aql');
dataParetoDynamicAql = load('distributions/pareto/dynamic/pareto_dynamic_aql');
dataParetoSizeAql    = load('distributions/pareto/size/pareto_size_aql');

% first line keeps averages comes from descriptive statistics
% average waiting time, average slowdown
randomWaiting = [data1random(1, 1), data2random(1, 1), data3random(1, 1), data4random(1, 1), data5random(1, 1), data6random(1, 1), data7random(1, 1), data8random(1, 1), data9random(1, 1)];
roundWaiting = [data1round(1, 1), data2round(1, 1), data3round(1, 1), data4round(1, 1), data5round(1, 1), data6round(1, 1), data7round(1, 1), data8round(1, 1), data9round(1, 1)];
dynamicWaiting = [data1dynamic(1, 1), data2dynamic(1, 1), data3dynamic(1, 1), data4dynamic(1, 1), data5dynamic(1, 1), data6dynamic(1, 1), data7dynamic(1, 1), data8dynamic(1, 1), data9dynamic(1, 1)];
sizeWaiting = [data1size(1, 1), data2size(1, 1), data3size(1, 1), data4size(1, 1), data5size(1, 1), data6size(1, 1), data7size(1, 1), data8size(1, 1), data9size(1, 1)];

randomSlow = [data1random(1, 2), data2random(1, 2), data3random(1, 2), data4random(1, 2), data5random(1, 2), data6random(1, 2), data7random(1, 2), data8random(1, 2), data9random(1, 2)];
roundSlow = [data1round(1, 2), data2round(1, 2), data3round(1, 2), data4round(1, 2), data5round(1, 2), data6round(1, 2), data7round(1, 2), data8round(1, 2), data9round(1, 2)];
dynamicSlow = [data1dynamic(1, 2), data2dynamic(1, 2), data3dynamic(1, 2), data4dynamic(1, 2), data5dynamic(1, 2), data6dynamic(1, 2), data7dynamic(1, 2), data8dynamic(1, 2), data9dynamic(1, 2)];
sizeSlow = [data1size(1, 2), data2size(1, 2), data3size(1, 2), data4size(1, 2), data5size(1, 2), data6size(1, 2), data7size(1, 2), data8size(1, 2), data9size(1, 2)];

% test shape parameters for bounded pareto distribution
a = [];
for i=1:9
	a = [a, 1+i/10];
end

% defined bin sizes for a task that can request  
taskSizes = [];
for i=1:2000
	taskSizes = [taskSizes, i*50];
end

%
% start of the graphs
%

% average waiting time graph
% all policies in the same graph
hold on;
plot(a, randomWaiting, '-+k', 'markerSize', 10, 'linewidth', 2);   
plot(a, roundWaiting, '-*r', 'markerSize', 10, 'linewidth', 2);   
plot(a, dynamicWaiting, '-og', 'markerSize', 10, 'linewidth', 2);   
plot(a, sizeWaiting, '-xb', 'markerSize', 10, 'linewidth', 2);   

legend('Random', 'Round Robin', 'Dynamic', 'Size Based', 'location', 'northeast');

title('Average Waiting Time vs Task Variability', 'fontsize', 20);
xlabel('Task Variability (Lower variates more)', 'fontsize', 20);
ylabel('Average Waiting Time in terms of simulation time unit', 'fontsize', 20);

print('graphs/paretoAverageWaitingTime.ps');
print('graphs/paretoAverageWaitingTime.png');
hold off;
clf;

% average slow down graph 
hold on;
plot(a, randomSlow, '-+k', 'markerSize', 10, 'linewidth', 2);   
plot(a, roundSlow, '-*r', 'markerSize', 10, 'linewidth', 2);   
plot(a, dynamicSlow, '-og', 'markerSize', 10, 'linewidth', 2);   
plot(a, sizeSlow, '-xb', 'markerSize', 10, 'linewidth', 2);   

legend('Random', 'Round Robin', 'Dynamic', 'Size Based', 'location', 'northeast');

title('Average Slow Down vs Task Variability', 'fontsize', 20);
xlabel('Task Variability (Lower variates more)', 'fontsize', 20);
ylabel('Average Slow Down', 'fontsize', 20);

print('graphs/paretoAverageSlowDown.ps');
print('graphs/paretoAverageSlowDown.png');
hold off;
clf;

%
% average queue length graphs
%
% exponential
hold on;
plot([1:8], dataExpAql(1, :), '-+k', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataExpAql(2, :), '-*r', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataExpAql(3, :), '-og', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataExpAql(4, :), '-xb', 'markerSize', 10, 'linewidth', 2);   

legend('Random', 'Round Robin', 'Size Based', 'Dynamic', 'location', 'northeast');

title('Average Queue Length of Hosts (Exponential)', 'fontsize', 20);
xlabel('Host Number', 'fontsize', 20);
ylabel('Average Queue Length', 'fontsize', 20);

axis([1, 8, 0, 60]);

print('graphs/aqlGraphs/exponential_aql.ps');
print('graphs/aqlGraphs/exponential_aql.png');
hold off;
clf;

% pareto 1.1
hold on
plot([1:8], dataParetoRandomAql(1, :), '-+k', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataParetoRoundAql(1, :), '-*r', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataParetoSizeAql(1, :), '-og', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataParetoDynamicAql(1, :), '-xb', 'markerSize', 10, 'linewidth', 2);   

legend('Random', 'Round Robin', 'Size Based', 'Dynamic', 'location', 'northeast');

title('Average Queue Length of Hosts (Pareto a=1.1)', 'fontsize', 20);
xlabel('Host Number', 'fontsize', 20);
ylabel('Average Queue Length', 'fontsize', 20);

axis([1, 8, 0, 60]);

print('graphs/aqlGraphs/pareto_1_1_aql.ps');
print('graphs/aqlGraphs/pareto_1_1_aql.png');
hold off;
clf;

% pareto 1.9
hold on
plot([1:8], dataParetoRandomAql(9, :), '-+k', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataParetoRoundAql(9, :), '-*r', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataParetoSizeAql(9, :), '-og', 'markerSize', 10, 'linewidth', 2);   
plot([1:8], dataParetoDynamicAql(9, :), '-xb', 'markerSize', 10, 'linewidth', 2);   

legend('Random', 'Round Robin', 'Size Based', 'Dynamic', 'location', 'northeast');

title('Average Queue Length of Hosts (Pareto a=1.9)', 'fontsize', 20);
xlabel('Host Number', 'fontsize', 20);
ylabel('Average Queue Length', 'fontsize', 20);

axis([1, 8, 0, 60]);

print('graphs/aqlGraphs/pareto_1_9_aql.ps');
print('graphs/aqlGraphs/pareto_1_9_aql.png');
hold off;
clf;



%
% bin average waiting time graphs
%
% most variable task size 
plot(taskSizes, data1random(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=RANDOM, a=1.1');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 275000]);

print('graphs/binGraphs/waitingTime/bin1RandomWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin1RandomWaitingTime.png');

plot(taskSizes, data1round(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=ROUNDROBIN, a=1.1');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 275000]);

print('graphs/binGraphs/waitingTime/bin1RoundWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin1RoundWaitingTime.png');

plot(taskSizes, data1dynamic(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=DYNAMIC, a=1.1');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 275000]);

print('graphs/binGraphs/waitingTime/bin1DynamicWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin1DynamicWaitingTime.png');

plot(taskSizes, data1size(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=SIZEBASED, a=1.1');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 275000]);

print('graphs/binGraphs/waitingTime/bin1SizeWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin1SizeWaitingTime.png');

% least variable task size
plot(taskSizes, data9random(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=RANDOM, a=1.9');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 36000]);

print('graphs/binGraphs/waitingTime/bin9RandomWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin9RandomWaitingTime.png');

plot(taskSizes, data9round(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=ROUNDROBIN, a=1.9');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 36000]);

print('graphs/binGraphs/waitingTime/bin9RoundWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin9RoundWaitingTime.png');

plot(taskSizes, data9dynamic(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=DYNAMIC, a=1.9');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 36000]);

print('graphs/binGraphs/waitingTime/bin9DynamicWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin9DynamicWaitingTime.png');

plot(taskSizes, data9size(2:end,1), 'linewidth', 3);
title('Average Waiting Times in term of Task Size, Policy=SIZEBASED, a=1.9');
xlabel('Task Size');
ylabel('Average Waiting Time in simulation units');
axis([0, 100000, 0, 36000]);

print('graphs/binGraphs/waitingTime/bin9SizeWaitingTime.ps');
print('graphs/binGraphs/waitingTime/bin9SizeWaitingTime.png');

%
% bin slow down graphs
%

% most variable task size
plot(taskSizes, data1random(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=RANDOM, a=1.1');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 285]);

print('graphs/binGraphs/slowDown/bin1RandomSlowDown.ps');
print('graphs/binGraphs/slowDown/bin1RandomSlowDown.png');

plot(taskSizes, data1round(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=ROUNDROBIN, a=1.1');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 285]);

print('graphs/binGraphs/slowDown/bin1RoundSlowDown.ps');
print('graphs/binGraphs/slowDown/bin1RoundSlowDown.png');

plot(taskSizes, data1dynamic(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=DYNAMIC, a=1.1');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 285]);

print('graphs/binGraphs/slowDown/bin1DynamicSlowDown.ps');
print('graphs/binGraphs/slowDown/bin1DynamicSlowDown.png');

plot(taskSizes, data1size(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=SIZEBASED, a=1.1');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 285]);

print('graphs/binGraphs/slowDown/bin1SizeSlowDown.ps');
print('graphs/binGraphs/slowDown/bin1SizeSlowDown.png');

% least variable task size
plot(taskSizes, data9random(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=RANDOM, a=1.9');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 20]);

print('graphs/binGraphs/slowDown/bin9RandomSlowDown.ps');
print('graphs/binGraphs/slowDown/bin9RandomSlowDown.png');

plot(taskSizes, data9round(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=ROUNDROBIN, a=1.9');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 20]);

print('graphs/binGraphs/slowDown/bin9RoundSlowDown.ps');
print('graphs/binGraphs/slowDown/bin9RoundSlowDown.png');

plot(taskSizes, data9dynamic(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=DYNAMIC, a=1.9');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 20]);

print('graphs/binGraphs/slowDown/bin9DynamicSlowDown.ps');
print('graphs/binGraphs/slowDown/bin9DynamicSlowDown.png');

plot(taskSizes, data9size(2:end,2), 'linewidth', 3);
title('Average Slow Down in term of Task Size, Policy=SIZEBASED, a=1.9');
xlabel('Task Size');
ylabel('Average Slow Down');
axis([0, 100000, 0, 20]);

print('graphs/binGraphs/slowDown/bin9SizeSlowDown.ps');
print('graphs/binGraphs/slowDown/bin9SizeSlowDown.png');