package com.ferhatelmas.perfeval.mini.run;

import com.ferhatelmas.perfeval.mini.simulate.Host;
import com.ferhatelmas.perfeval.mini.simulate.Policy;

public abstract class Configuration {

    protected int hostNumber;
    protected int meanOfDistribution;
    protected double load;
    protected int numberOfTasks;
    protected Policy policy;
    protected double lambda;
    protected Host[] hosts;
    protected int[] sizeLimits;

    public Configuration(int hostNumber, int meanOfDistribution, double load, int numberOfTasks, Policy policy) {
        this.hostNumber = hostNumber;
        this.meanOfDistribution = meanOfDistribution;
        this.load = load;
        this.numberOfTasks = numberOfTasks;
        this.policy = policy;
        this.lambda = (hostNumber * load) / meanOfDistribution;
        this.hosts = new Host[hostNumber];
        for(int i=0; i<hosts.length; i++) {
            hosts[i] = new Host();
        }
        this.sizeLimits = new int[hostNumber+1];
    }

    public int getHostNumber() {
        return hostNumber;
    }

    public int getMeanOfDistribution() {
        return meanOfDistribution;
    }

    public double getLoad() {
        return load;
    }

    public int getNumberOfTasks() {
        return numberOfTasks;
    }

    public Policy getPolicy() {
        return policy;
    }

    public double getLambda() {
        return lambda;
    }

    public Host[] getHosts() {
        return hosts;
    }

    public int[] getSizeLimits() {
        return sizeLimits;
    }

    public abstract int getHigherBound();

    public abstract int getLowerBound();

    public abstract int getNextTaskSize(double random);
}
