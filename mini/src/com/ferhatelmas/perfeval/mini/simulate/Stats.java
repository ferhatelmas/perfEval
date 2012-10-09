package com.ferhatelmas.perfeval.mini.simulate;

public class Stats {

    private double waitingTimes;
    private double[] waitingTimeClasses;

    private double slowDown;
    private double[] slowDownClasses;

    private double[] averageQueueLengths;

    public Stats(double waitingTimes, double[] waitingTimeClasses,
                 double slowDown, double[] slowDownClasses,
                 double[] averageQueueLengths) {
        this.waitingTimes = waitingTimes;
        this.waitingTimeClasses = waitingTimeClasses;
        this.slowDown = slowDown;
        this.slowDownClasses = slowDownClasses;
        this.averageQueueLengths = averageQueueLengths;
    }

    public double getWaitingTimes() {
        return waitingTimes;
    }

    public double[] getWaitingTimeClasses() {
        return waitingTimeClasses;
    }

    public double getSlowDown() {
        return slowDown;
    }

    public double[] getSlowDownClasses() {
        return slowDownClasses;
    }

  public double[] getAverageQueueLengths() {
    return averageQueueLengths;
  }

  public void setAverageQueueLengths(double[] averageQueueLengths) {
    this.averageQueueLengths = averageQueueLengths;
  }

  public void addStats(Stats s) {
        waitingTimes += s.getWaitingTimes();
        slowDown += s.getSlowDown();

        double[] wtc = s.getWaitingTimeClasses();
        double[] sdc = s.getSlowDownClasses();

        for(int i=0; i<wtc.length; i++) {
            waitingTimeClasses[i] += wtc[i];
            slowDownClasses[i] += sdc[i];
        }
    }

    public void normalize(int cnt) {
        waitingTimes /= cnt;
        slowDown /= cnt;

        for(int i=0; i<waitingTimeClasses.length; i++) {
            waitingTimeClasses[i] /= cnt;
            slowDownClasses[i] /= cnt;
        }
    }
}
