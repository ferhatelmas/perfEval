% Clear and clean everything
clear all; close all;

% General parameters setting
maxReq = 10000;
intensity = [50, 70];

% Performs maxLoop iteration of the simpleServer
for loop=1:40
  
    fprintf('=> loop: %d ', loop);

    % Perform one simulation of the simple server
    stat = simpleServer(maxReq, intensity(1));

    % compute the final statistics
    mean50Task1(loop,:) = mean(stat.arrivalTask1-stat.requestTask1);
    mean50Task2(loop,:) = mean(stat.arrivalTask2-stat.requestTask2);
  
    fprintf('mean50Task1 = %f, mean50Task2 = %f\n', mean50Task1(loop), mean50Task2(loop));
  
end

fprintf('meanTotal50Task1 = %f, meanTotal50Task2 = %f\n', mean(mean50Task1), mean(mean50Task2));
fprintf('medianTotal50Task1 = %f, medianTotal50Task2 = %f\n', median(mean50Task1), median(mean50Task2));

% Performs maxLoop iteration of the simpleServer
for loop=1:40
  
    fprintf('=> loop: %d ', loop);

    % Perform one simulation of the simple server
    stat = simpleServer(maxReq, intensity(2));

     % compute the final statistics
    mean70Task1(loop,:) = mean(stat.arrivalTask1-stat.requestTask1);
    mean70Task2(loop,:) = mean(stat.arrivalTask2-stat.requestTask2);
  
    fprintf('mean70Task1 = %f, mean70Task2 = %f\n', mean70Task1(loop), mean70Task2(loop));
  
end

fprintf('meanTotal70Task1 = %f, meanTotal70Task2 = %f\n', mean(mean70Task1), mean(mean70Task2));
fprintf('medianTotal70Task1 = %f, medianTotal70Task2 = %f\n', median(mean70Task1), median(mean70Task2));