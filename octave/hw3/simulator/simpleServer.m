
function stat = simpleServer(maxReq, intensity)

% Initialize the random number generator
rand('state', sum(100*clock));

% Create the eventScheduler
% And bootstrap it (i.e. add the first event)
evSched.currentTime = -1;
evSched.firingTime = 0;
evSched.evList = []; % This is the event list, note that it is a [n x 2] vector.
		      % The first column corresponds to the firing time
		      % The second column is the event type
		      % 1 for an arrival
		      % 2 for a service
		      % 3 for a departure
evSched.evListLength = 0; % And a variable corresponding to the event list size (i.e number of rows)
evSched = addEvent(0, 1, 1, evSched); % Add first event i.e. 1 for arrival with firing Time = 0

% Initialize buffer queues
server.queue = [];
server.task = [];
server.queueLength = 0;
server.nbArrivalTask1 = 0;
server.nbArrivalTask2 = 0;
server.nbRequestTask1 = 0;
server.nbRequestTask2 = 0;

% Variables used for statistic purpose, refer to the perfeval lecture notes for the terminology (simulation chapter)
stat.eventTime = [];
stat.queueSize = [];
stat.arrivalTask1 = [];
stat.arrivalTask2 = [];
stat.requestTask1 = [];
stat.requestTask2 = [];
stat.queueLengthCtr = 0;
stat.responseTimeTask1Ctr = 0;
stat.responseTimeTask2Ctr = 0;

% Execute Events until the total number of arrivals reach maxReq
while server.nbArrivalTask1 <= maxReq

  % Get next event
  evSched.firingTime = evSched.evList(1, 1);
  evType = evSched.evList(1, 2); % Assumes that evList is sorted
  evTask = evSched.evList(1, 3);

  % Remove it from the event list
  evSched = delEvent(evSched);

  % Increment counter for the arrival queue
  stat.queueLengthCtr = ...
      stat.queueLengthCtr + server.queueLength*(evSched.firingTime-evSched.currentTime);
  
  % Register queue size
  stat.eventTime(end+1) = evSched.firingTime;
  stat.queueSize(end+1) = server.queueLength;
  stat.arrivalTask1(end+1) = server.nbArrivalTask1;
  stat.arrivalTask2(end+1) = server.nbArrivalTask2;
  stat.requestTask1(end+1)   = server.nbRequestTask1;
  stat.requestTask2(end+1)   = server.nbRequestTask2;
  
  % Execute current event
  % Since the data structures are not shared, the discrimination between
  % events is done here. Then, given the event type, there is a specific
  % execution function to call and a set of parameter to update
  switch evType

   case 1, % Arrival fprintf('=> Arrival Event: evFiringTime = %d\n',evSched.firingTime);

    % Create a new request and add it at the end of the buffer
    server.queue = [server.queue; evSched.firingTime];
    server.task  = [server.task; evTask];
    server.queueLength = server.queueLength + 1;
    
    if evTask == 1
      server.nbArrivalTask1 = server.nbArrivalTask1 + 1;
    else 
      server.nbArrivalTask2 = server.nbArrivalTask2 + 1;
    end

    % Add a service event if the queue was empty
    if server.queueLength == 1
      evSched = addEvent(evSched.firingTime, 2, evTask, evSched);
    end
    % Draw a random number from distrib F to create a new Arrival event
    if evTask == 1
      delta = getDelta('A', intensity);
      evSched = addEvent(evSched.firingTime+delta, 1, 1, evSched);
    end
    
   case 2, % Service fprintf('=> Service Event: evFiringTime = %d\n',evSched.firingTime);

    % Draw a random number from distrib G to create a new Departure event
    if evTask == 1
      delta = getDelta('1', intensity);
    else
      delta = getDelta('2', intensity);
    end
    evSched = addEvent(evSched.firingTime+delta, 3, evTask, evSched);

   case 3, % Departure fprintf('=> Departure Event: evFiringTime = %d\n',evSched.firingTime);
 
    % Update response time and request counters
    if evTask == 1
      stat.responseTimeTask1Ctr = stat.responseTimeTask1Ctr + evSched.firingTime-server.queue(1);
      server.nbRequestTask1 = server.nbRequestTask1 + 1;
      evSched = addEvent(evSched.firingTime, 1, 2, evSched);
    else
      % Update response time counters
      stat.responseTimeTask2Ctr = stat.responseTimeTask2Ctr + evSched.firingTime-server.queue(1);
      server.nbRequestTask2 = server.nbRequestTask2 + 1;
    end
    
    % Remove request from buffer and delete it
    server.queue = server.queue(2:end);
    server.task  = server.task(2:end);
    server.queueLength = server.queueLength - 1;
    
    % Add a service event if the queue is not empty
    if server.queueLength > 0
      evSched = addEvent(evSched.firingTime, 2, server.task(1), evSched);
    end

   otherwise
    fprintf('Bug: this type of event does not exist!\n');

  end

  % Set current time to ev.firingTime
  evSched.currentTime = evSched.firingTime;
end
