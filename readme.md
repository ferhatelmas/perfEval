## Performance Evaluation

My submissions for performance evaluation course.

_octave_ dir contains homework submissions.
* Hw5 was a competition to predict EPFL outgoing web traffic a week before, it got the highest score. 

_java_ dir contains some parts of the homeworks that are easier to write in Java, organized by package.

_mini_ dir contains my mini project proposal and re-performed simulation code and results for task assignment policy for distributed server system. Related paper and presented two slides can also be found.
* Main result of simulation is dynamic policy (assignment to host with least work left) is generally best. In low variability or exponential distribution in task sizes, it is mathematically proofed to be the best. However, in high variability size based policy (task are assigned to host according to predefined limits[expectation across host is equal] that host can admit jobs) is the best.   