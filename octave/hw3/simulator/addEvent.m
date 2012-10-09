
function evSched = addEvent(firingTime, eventType, taskType, evSched)

% Add the event
evSched.evList = [evSched.evList; 
		  firingTime, eventType, taskType];
evSched.evListLength = evSched.evListLength + 1;
% Sort the list
evSched.evList = sortrows(evSched.evList);
