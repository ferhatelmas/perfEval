% Clear and clean everything
clear all; close all;

% General parameters setting
maxReq = 10000;
intensity = 70;

% Performs maxLoop iteration of the simpleServer
for loop=1:40
  
    fprintf('=> loop: %d ', loop);

    % Perform one simulation of the simple server
    stat = simpleServer(maxReq, intensity);

     % compute the final statistics
    mean70Task1(loop,:) = mean(stat.arrivalTask1(45000:end)-stat.requestTask1(45000:end));
    mean70Task2(loop,:) = mean(stat.arrivalTask2(45000:end)-stat.requestTask2(45000:end));
  
    fprintf('mean70Task1 = %f, mean70Task2 = %f\n', mean70Task1(loop), mean70Task2(loop));
  
end

fprintf('meanTotal70Task1 = %f, meanTotal70Task2 = %f\n', mean(mean70Task1), mean(mean70Task2));
fprintf('medianTotal70Task1 = %f, medianTotal70Task2 = %f\n', median(mean70Task1), median(mean70Task2));