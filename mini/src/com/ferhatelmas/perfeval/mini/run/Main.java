package com.ferhatelmas.perfeval.mini.run;

import com.ferhatelmas.perfeval.mini.simulate.Policy;
import com.ferhatelmas.perfeval.mini.simulate.Simulator;
import com.ferhatelmas.perfeval.mini.simulate.Stats;
import org.apache.commons.math3.stat.descriptive.DescriptiveStatistics;

import java.io.BufferedReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;

public class Main {

    private static int runLength = 10;

    public static void main(String[] args) throws IOException {

      PrintWriter cpw = new PrintWriter(new FileWriter("confidence_intervals"));

      DescriptiveStatistics dsw, dss;
      HashMap<Integer, DescriptiveStatistics> waitingTimeStats, slowDownStats;
      HashMap<Integer, DescriptiveStatistics> averageQueueLengths;

      cpw.println("Exponential");
      for(Policy policy : Policy.values()) {

        dsw = new DescriptiveStatistics();
        dss = new DescriptiveStatistics();
        waitingTimeStats = new HashMap<Integer, DescriptiveStatistics>();
        slowDownStats    = new HashMap<Integer, DescriptiveStatistics>();
        averageQueueLengths = new HashMap<Integer, DescriptiveStatistics>();

        for(int r=0; r<runLength; r++) {
          Stats newStat = new Simulator(new ExponentialConfiguration(8, 300, 0.8, 10000000, policy, 100000)).simulate();
          dsw.addValue(newStat.getWaitingTimes());
          dss.addValue(newStat.getSlowDown());

          double[] wc = newStat.getWaitingTimeClasses();
          double[] sc = newStat.getSlowDownClasses();

          for(int i=0; i<wc.length; i++) {
            if(!waitingTimeStats.containsKey(i)) {
              waitingTimeStats.put(i, new DescriptiveStatistics());
              slowDownStats.put(i, new DescriptiveStatistics());
            }

            waitingTimeStats.get(i).addValue(wc[i]);
            slowDownStats.get(i).addValue(sc[i]);
          }

          double[] aql = newStat.getAverageQueueLengths();
          for(int i=0; i<aql.length; i++) {
            if(!averageQueueLengths.containsKey(i)) {
              averageQueueLengths.put(i, new DescriptiveStatistics());
            }

            averageQueueLengths.get(i).addValue(aql[i]);
          }
        }

        cpw.println(policy);
        cpw.format("%.5f\t%.5f\t%.5f\n", dsw.getMean(), dsw.getPercentile(.5), dsw.getStandardDeviation());
        cpw.format("%.5f\t%.5f\t%.5f\n", (-1.96 * dsw.getStandardDeviation() + dsw.getMean()), dsw.getMean(), (1.96 * dsw.getStandardDeviation() + dsw.getMean()));
        for(int i : averageQueueLengths.keySet()) {
          cpw.format("%.5f\t", averageQueueLengths.get(i).getMean());
        }
        cpw.println();

        PrintWriter pw = new PrintWriter(new FileWriter("exponential_" + policy));
        pw.format("%.5f\t%.5f\n", dsw.getMean(), dss.getMean());

        for(int i : waitingTimeStats.keySet()) {
          pw.format("%.5f\t%.5f\n", waitingTimeStats.get(i).getMean(), slowDownStats.get(i).getMean());
        }
        pw.close();
      }

      cpw.println("\nPareto");
      for(Policy policy : Policy.values()) {
        cpw.println(policy);
        for(double a=1.1; a<2; a+=0.1) {

          dsw = new DescriptiveStatistics();
          dss = new DescriptiveStatistics();
          waitingTimeStats = new HashMap<Integer, DescriptiveStatistics>();
          slowDownStats    = new HashMap<Integer, DescriptiveStatistics>();
          averageQueueLengths = new HashMap<Integer, DescriptiveStatistics>();

          for(int r=0; r<runLength; r++) {
            Stats newStat = new Simulator(new ParetoConfiguration(8, 300, 0.8, 10000000, policy, a, 100000)).simulate();
            dsw.addValue(newStat.getWaitingTimes());
            dss.addValue(newStat.getSlowDown());

            double[] wc = newStat.getWaitingTimeClasses();
            double[] sc = newStat.getSlowDownClasses();

            for(int i=0; i<wc.length; i++) {
              if(!waitingTimeStats.containsKey(i)) {
                waitingTimeStats.put(i, new DescriptiveStatistics());
                slowDownStats.put(i, new DescriptiveStatistics());
              }

              waitingTimeStats.get(i).addValue(wc[i]);
              slowDownStats.get(i).addValue(sc[i]);
            }

            double[] aql = newStat.getAverageQueueLengths();
            for(int i=0; i<aql.length; i++) {
              if(!averageQueueLengths.containsKey(i)) {
                averageQueueLengths.put(i, new DescriptiveStatistics());
              }

              averageQueueLengths.get(i).addValue(aql[i]);
            }
          }
          cpw.format("%.1f\n%.5f\t%.5f\t%.5f\n", a, dsw.getMean(), dsw.getPercentile(.5), dsw.getStandardDeviation());
          cpw.format("%.5f\t%.5f\t%.5f\n", (-1.96*dsw.getStandardDeviation()+dsw.getMean()), dsw.getMean(), (1.96*dsw.getStandardDeviation()+dsw.getMean()));
          for(int i : averageQueueLengths.keySet()) {
            cpw.format("%.5f\t", averageQueueLengths.get(i).getMean());
          }
          cpw.println();

          PrintWriter pw = new PrintWriter(new FileWriter("pareto_" + String.format("%.1f", a) + "_" + policy));
          pw.format("%.5f\t%.5f\n", dsw.getMean(), dss.getMean());

          for(int i : waitingTimeStats.keySet()) {
            pw.format("%.5f\t%.5f\n", waitingTimeStats.get(i).getMean(), slowDownStats.get(i).getMean());
          }
          pw.close();
        }
        cpw.println();
      }
      cpw.close();
    }

}
