package com.ferhatelmas.perfeval.mini.simulate;

public class Task implements Comparable {

    public enum STATE{ARRIVAL, SERVICE, DEPARTURE}

    private int size;
    private int remaining;
    private Host host;

    private long firingTime;
    private STATE state;

    private int counter;

    public Task(int size, long firingTime, STATE state) {
        this.size = size;
        this.remaining = size;
        this.firingTime = firingTime;
        this.state = state;
        this.counter = 0;
    }

    @Override
    public int compareTo(Object o) {
        Task t = (Task)o;

        if(this.firingTime < t.firingTime) return -1;
        else if(this.firingTime == t.firingTime) return 0;
        else return 1;
    }

    public int getCounter() {
        return counter;
    }

    public void setCounter(int counter) {
        this.counter = counter;
    }

    public int getSize() {
        return size;
    }

    public void setSize(int size) {
        this.size = size;
    }

    public int getRemaining() {
        return remaining;
    }

    public void setRemaining(int remaining) {
        this.remaining = remaining;
    }

    public Host getHost() {
        return host;
    }

    public void setHost(Host host) {
        this.host = host;
    }

    public long getFiringTime() {
        return firingTime;
    }

    public void setFiringTime(long firingTime) {
        this.firingTime = firingTime;
    }

    public STATE getState() {
        return state;
    }

    public void setState(STATE state) {
        this.state = state;
    }
}
