package com.ferhatelmas.perfeval.mini.run;

import com.ferhatelmas.perfeval.mini.simulate.Policy;

public class ExponentialConfiguration extends Configuration {

    private int higherBound;
    private int lowerBound;

    public ExponentialConfiguration(int hostNumber, int meanOfDistribution, double load, int numberOfTasks, Policy policy,
                                    int higherBound) {
        super(hostNumber, meanOfDistribution, load, numberOfTasks, policy);
        this.lowerBound = 0;
        this.higherBound = higherBound;
        if(policy == Policy.SIZE_BASED) fillSizeLimits();
    }

    public void fillSizeLimits() {
        int portion = meanOfDistribution / hostNumber;

        sizeLimits[0] = lowerBound;
        for(int i=1; i<hostNumber; i++) {
            sizeLimits[i] = getNextLimit(portion, sizeLimits[i - 1]);
        }
        sizeLimits[hostNumber] = higherBound;
    }

    private int getNextLimit(int portion, int start) {
        double sum = 0;
        int i = start;
        while(sum < portion) {
            sum += (i * (1.0/meanOfDistribution) * Math.exp(-(1.0/meanOfDistribution)*i));
            i++;
        }
        return i;
    }

    @Override
    public int getHigherBound() {
        return higherBound;
    }

    @Override
    public int getLowerBound() {
        return lowerBound;
    }

    @Override
    public int getNextTaskSize(double random) {
        int size = (int)(-Math.log(random) * meanOfDistribution);
        /* This fix is required since
        * although for exponential 0 is valid, for size, 0 is invalid,
        * 1 is the minimum task size, otherwise we can get infinity values for slowdown
        * */
        return size > 0 ? size : 1;
    }
}
