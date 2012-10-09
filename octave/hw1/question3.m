strings_array = {'Theta', 'Packets Per Second', 'Collision Per Second', 'Delay'};
strings = cellstr(strings_array);

acronyms_array = {'theta', 'pps', 'cps', 'd'};
acronyms = cellstr(acronyms_array);

data = load('question34.data');

oneServer = [];
doubleServer = [];

for i=1:length(data)
	if(data(i, 2) == 1 && data(i, 3) <= 2)
		if(data(i, 3) == 1)
			oneServer = [oneServer; data(i, 1), data(i, 4), data(i, 5), data(i, 6), data(i, 7)];
		else
			doubleServer = [doubleServer; data(i, 1), data(i, 4), data(i, 5), data(i, 6), data(i, 7)];
		end
	end
end

for i=1:length(strings)
	plot(oneServer(:, 1), oneServer(:, i+1), 'rx', 'MarkerSize', 10);
	xlabel('# of Customers');
	ylabel(char(strings(i)));
	title(cstrcat(char(strings(i)), ' vs Customers [AP:1, S:1]'));
	print(cstrcat('question3_1_', char(acronyms(i)), '.ps'));
	clf;

	plot(doubleServer(:, 1), doubleServer(:, i+1), 'rx', 'MarkerSize', 10);
	xlabel('# of Customers');
	ylabel(char(strings(i)));
	title(cstrcat(char(strings(i)), ' vs Customers [AP:1, S:2]'));
	print(cstrcat('question3_2_', char(acronyms(i)), '.ps'));
	clf;
end
