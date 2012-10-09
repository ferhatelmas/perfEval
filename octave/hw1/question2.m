strings_array = {'Theta', 'Packets Per Second', 'Collision Per Second', 'Delay'};
strings = cellstr(strings_array);

acronyms_array = {'theta', 'pps', 'cps', 'd'};
acronyms = cellstr(acronyms_array);

data = load('question2.data');

for i=1:length(strings)
	plot(data(:, 1), data(:, i+1), 'rx', 'MarkerSize', 10);
	xlabel('# of Customers');
	ylabel(char(strings(i)));
	title(cstrcat(char(strings(i)), ' vs Customers [AP:1, S:1]'));
	print(cstrcat('question2_', char(acronyms(i)), '.ps'));
	clf;
end
