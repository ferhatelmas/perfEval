% Clear and clean everything
clear all; close all;

% General parameters setting
maxReq = 10000;
maxLoop = 10;
intensity = 50;

% Performs maxLoop iteration of the simpleServer
for loop=1:1:maxLoop

  fprintf('=> loop: %d ', loop);

  % Perform one simulation of the simple server
  stat = simpleServer(maxReq, intensity);

  % compute the final statistics
  meanTask1ResponseTime(loop,:) = stat.responseTimeTask1Ctr/stat.requestTask1(end);
  meanTask2ResponseTime(loop,:) = stat.responseTimeTask2Ctr/stat.requestTask2(end);
  meanTask1Service(loop, :) = (stat.requestTask1(end)/stat.eventTime(end)) * 1000;
  meanTask2Service(loop, :) = (stat.requestTask2(end)/stat.eventTime(end)) * 1000;
  
  fprintf('meanTask1ResponseTime = %f, meanTask2ResponseTime = %f\n', ... 
    meanTask1ResponseTime(loop), meanTask2ResponseTime(loop));
  fprintf('meanTask1Service = %f, meanTask2Service = %f\n', ... 
    meanTask1Service(loop), meanTask2Service(loop));

end
fprintf('Response times task1 = %f, task2 = %f\n', mean(meanTask1ResponseTime), mean(meanTask2ResponseTime)); 
fprintf('Service numbers task1 = %f, task2 = %f\n', mean(meanTask1Service), mean(meanTask2Service));