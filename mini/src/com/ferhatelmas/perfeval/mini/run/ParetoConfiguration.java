package com.ferhatelmas.perfeval.mini.run;

import com.ferhatelmas.perfeval.mini.simulate.Policy;

public class ParetoConfiguration extends Configuration {

    private double a;
    private int higherBound;
    private int lowerBound;

    public ParetoConfiguration(int hostNumber, int meanOfDistribution, double load, int numberOfTasks, Policy policy,
                               double a, int higherBound) {
        super(hostNumber, meanOfDistribution, load, numberOfTasks, policy);
        this.a = a;
        this.higherBound = higherBound;
        this.lowerBound = calculateLowerBound();
        if(policy == Policy.SIZE_BASED) fillSizeLimits();
    }

    public void fillSizeLimits() {
        int portion = meanOfDistribution / hostNumber;

        double c = a * Math.pow(lowerBound, a);
        double d = 1 - Math.pow(lowerBound/(double)higherBound, a);

        sizeLimits[0] = lowerBound;
        for(int i=1; i<hostNumber; i++) {
            sizeLimits[i] = getNextLimit(portion, sizeLimits[i - 1], c, d);
        }
        sizeLimits[hostNumber] = higherBound;
    }

    private int getNextLimit(int portion, int start, double c, double d) {
        double sum = 0;
        int i = start;
        while(sum < portion) {
            sum += (i * c * Math.pow(i, -a-1)) / d;
            i++;
        }
        return i;
    }

    private int calculateLowerBound() {
        double c = meanOfDistribution * (a-1)/a;
        double powerOfP = Math.pow(higherBound, a);
        double firstMul = c-higherBound;

        int k = -1, min = Integer.MAX_VALUE;
        for(int i=0; i<=meanOfDistribution; i++) {
            int tmp = (int)Math.abs(Math.pow(i, a)*firstMul + (i-c)*powerOfP);
            if(tmp < min) {
                min = tmp;
                k = i;
            }
        }
        return k;
    }

    public double getA() {
        return a;
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
        return (int)Math.pow(-((random-1)*Math.pow(higherBound, a) - random*Math.pow(lowerBound, a)) /
                (Math.pow(higherBound*lowerBound, a)), -1/a);
    }
}
