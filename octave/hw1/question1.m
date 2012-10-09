strings_array = {'Theta', 'Packets Per Second', 'Collision Per Second', 'Delay'};
strings = cellstr(strings_array);

acronyms_array = {'theta', 'pps', 'cps', 'd'};
acronyms = cellstr(acronyms_array);

data = load('question1.data');

for i=1:length(strings)
	plot(data(:, i), 'rx', 'MarkerSize', 10);
	xlabel('Runs');
	ylabel(char(strings(i)));
	title(cstrcat(char(strings(i)), ' vs Runs [C:1, AP:1, S:1]'));
	print(cstrcat('question1_', char(acronyms(i)), '.ps'));
	clf;
end
