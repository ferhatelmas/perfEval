package com.ferhatelmas.perfeval.mini.simulate;

import com.ferhatelmas.perfeval.mini.run.Configuration;
import com.ferhatelmas.perfeval.mini.util.Util;

import java.util.Collections;
import java.util.LinkedList;
import java.util.Random;

public class Simulator {

    private long clock;
    private Random r;
    private LinkedList<Task> tasks;

    private long totalWaitingTime;
    private int numberOfServiced;

    private int classRange;
    private int[] classCounters;
    private long[] waitingTimeClasses;
    private long[] slowDownClasses;

    private Configuration configuration;

    public Simulator(Configuration configuration) {

        this.configuration = configuration;
        this.tasks = new LinkedList<Task>();
        this.clock = -1;
        this.r = new Random(Util.nextPrime(System.currentTimeMillis()));

        this.totalWaitingTime = 0;
        this.numberOfServiced = 0;

        this.classRange = 50;
        int classCount = configuration.getHigherBound() / classRange;
        this.waitingTimeClasses  = new long[classCount];
        this.slowDownClasses = new long[classCount];
        this.classCounters = new int[classCount];
    }

    public Stats simulate() {
        int numberOfArrivedTasks = 0;

        Task firstTask = new Task(getNextTaskSize(), 0, Task.STATE.ARRIVAL);
        tasks.add(firstTask);

        while(numberOfArrivedTasks <= configuration.getNumberOfTasks()) {

            Task t = tasks.removeFirst();
            clock = t.getFiringTime();
            for(Host h : configuration.getHosts()) {
              h.updateQueueTime(clock);
            }

            if(t.getState() == Task.STATE.ARRIVAL) {

                numberOfArrivedTasks++;

                t.setCounter(numberOfArrivedTasks);

                schedule(t);

                if(t.getHost().getNumberOfTasks() == 1 && t.getHost().isServiceEmpty()) {
                    t.setState(Task.STATE.SERVICE);
                    addTask(t);
                    if(numberOfArrivedTasks > configuration.getNumberOfTasks()/5) {
                        numberOfServiced++;

                      int classIndex = t.getSize()/classRange;
                      // waitingTimeClasses[classIndex] isn't updated since task is serviced as soon as it arrives
                      slowDownClasses[classIndex] += t.getSize();
                      classCounters[classIndex]++;
                    }
                }

                addTask(new Task(getNextTaskSize(), t.getFiringTime() + getNextTaskArrivalTime(), Task.STATE.ARRIVAL));

            } else if(t.getState() == Task.STATE.SERVICE) {

                t.setFiringTime(t.getFiringTime() + t.getSize());
                t.setState(Task.STATE.DEPARTURE);
                t.getHost().service(numberOfArrivedTasks >= configuration.getNumberOfTasks()/5);
                addTask(t);

            } else {

                Host host = t.getHost();
                host.departure();
                if(host.getNumberOfTasks() > 0) {
                    Task waitingTask = host.getWaitingTask();
                    if(numberOfArrivedTasks > configuration.getNumberOfTasks()/5) {
                        totalWaitingTime += t.getFiringTime()-waitingTask.getFiringTime();
                        numberOfServiced++;

                        int classIndex = waitingTask.getSize()/classRange;
                        waitingTimeClasses[classIndex] += t.getFiringTime()-waitingTask.getFiringTime();
                        slowDownClasses[classIndex] += waitingTask.getSize();
                        classCounters[classIndex]++;
                    }
                    waitingTask.setFiringTime(t.getFiringTime());
                    waitingTask.setState(Task.STATE.SERVICE);
                    addTask(waitingTask);
                }

            }

        }

        return calculateStats();
    }

    private Stats calculateStats() {
        double[] averageQueueLengths = new double[configuration.getHostNumber()];
        for(int i=0; i<averageQueueLengths.length; i++) {
          averageQueueLengths[i] = configuration.getHosts()[i].getAverageQueueLength(clock);
        }

        double[] waitingTimes = new double[classCounters.length];
        double[] slowDowns = new double[classCounters.length];
        for(int i=0; i<classCounters.length; i++) {
            if(classCounters[i] == 0) {
                waitingTimes[i] = 0;
                slowDowns[i] = 0;
            } else {
                waitingTimes[i] = waitingTimeClasses[i] / (double)classCounters[i];
                slowDowns[i]    = waitingTimeClasses[i] / (double)slowDownClasses[i];
            }
        }

        return new Stats(totalWaitingTime/(double)numberOfServiced, waitingTimes,
                totalWaitingTime/(double)getTotalServiceTime(clock), slowDowns,
                averageQueueLengths);
    }

    private long getTotalServiceTime(long clock) {
        long sum = 0;
        for(Host host : configuration.getHosts()) {
            sum += host.getServiceTime(clock);
        }
        return sum;
    }

    private void schedule(Task task) {
        if(configuration.getPolicy() == Policy.DYNAMIC) {

            task.setHost(getHostWithMinWork());

        } else if(configuration.getPolicy() == Policy.RANDOM) {

            double step = 1.0/ configuration.getHostNumber();
            double random = r.nextDouble();
            for(int i=0; i< configuration.getHostNumber(); i++) {
                if(random >= step*i && random < step*(i+1)) {
                    task.setHost(configuration.getHosts()[i]);
                    break;
                }
            }

        } else if(configuration.getPolicy() == Policy.ROUND_ROBIN) {

            task.setHost(configuration.getHosts()[task.getCounter()% configuration.getHostNumber()]);

        } else {

            int size = task.getSize();
            for(int i=0; i< configuration.getHostNumber(); i++) {
                if(size >= configuration.getSizeLimits()[i] && size < configuration.getSizeLimits()[i+1]) {
                    task.setHost(configuration.getHosts()[i]);
                    break;
                }
            }

        }

        task.getHost().assign(task);
    }

    private Host getHostWithMinWork() {
        long min = Long.MAX_VALUE;
        Host host = null;
        for(Host h : configuration.getHosts()) {
            long remaining = h.getRemainingSize(clock);
            if(remaining < min) {
                min = remaining;
                host = h;
            }
        }
        return host;
    }

    private void addTask(Task task) {
        tasks.add(task);
        Collections.sort(tasks);
    }

    private int getNextTaskArrivalTime() {
        return (int)(-Math.log(r.nextDouble()) / configuration.getLambda());
    }

    private int getNextTaskSize() {
        return configuration.getNextTaskSize(r.nextDouble());
    }

}
