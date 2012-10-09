package com.ferhatelmas.perfeval.mini.simulate;

import java.util.LinkedList;

public class Host {

    private int numberOfTasks;
    private long totalSize;

    private LinkedList<Task> queue;

    private Task servicedTask;

    private long serviceTime;

    private long prevQueueUpdateTime;
    private long totalQueueTime;

    public Host() {
        this.queue = new LinkedList<Task>();
        this.numberOfTasks = 0;
        this.totalSize = 0;
        this.servicedTask = null;
        this.serviceTime = 0;
        prevQueueUpdateTime = 0;
        totalQueueTime = 0;
    }

    public void service(boolean isStationary) {
        this.servicedTask = queue.removeFirst();
        numberOfTasks--;
        totalSize -= servicedTask.getSize();
        if(isStationary) serviceTime += servicedTask.getSize();
    }

    public void departure() {
        this.servicedTask = null;
    }

    public void assign(Task t) {
        queue.add(t);
        numberOfTasks++;
        totalSize += t.getSize();
    }

    public Task getWaitingTask() {
        return queue.getFirst();
    }

    public boolean isServiceEmpty() {
        return servicedTask == null;
    }

    public long getServiceTime(long clock) {
        if(servicedTask != null) {
            serviceTime -= servicedTask.getFiringTime() - clock;
        }
        return serviceTime;
    }

    public int getNumberOfTasks() {
        return numberOfTasks;
    }

    public long getTotalSize() {
        return totalSize;
    }

    public long getRemainingSize(long clock) {
        return totalSize + (servicedTask == null ? 0 : servicedTask.getFiringTime()-clock);
    }

    public void updateQueueTime(long clock) {
      totalQueueTime += queue.size()*(clock-prevQueueUpdateTime);
      prevQueueUpdateTime = clock;
    }

    public double getAverageQueueLength(long clock) {
      return totalQueueTime / (double)clock;
    }

}
