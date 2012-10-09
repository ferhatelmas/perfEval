% Clear and clean everything
clear all; close all;

% General parameters setting
maxReq = 10000;
low = 30;
high = 150;

% Performs maxLoop iteration of the simpleServer
for loop=30:10:150

  fprintf('=> loop: %d ', loop);

  % Perform one simulation of the simple server
  stat = simpleServer(maxReq, loop);

  % compute the final statistics
  meanQueueLength(loop/10-2,:) = stat.queueLengthCtr/stat.eventTime(end);
  
  fprintf('meanQueueLength = %f\n', meanQueueLength(loop/10-2));

end

fprintf('Mean Queue Length = %f\n', mean(meanQueueLength));

plot(low:10:high, meanQueueLength);
xlabel('Intensity of the arrivals');
ylabel('Mean Queue Length');
title('Mean Queue Length vs Intensity of Task Arrivals')
print -f1 -r600 -depsc2 question4.eps;
