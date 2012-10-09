
function evSched = delEvent(evSched)

% Remove the event at the top of the queue
% We do not have to sort the list, already sorted
evSched.evList = evSched.evList(2:end,:);
evSched.evListLength = evSched.evListLength-1;

%fprintf('Event deleted, evList length = %d\n',evSched.evListLength);
