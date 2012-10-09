package com.ferhatelmas.perfeval.mini.test;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.Scanner;

public class Test {

  public static void main(String[] args) throws IOException {

    Scanner in = new Scanner(new FileInputStream("paretoRandoms"));

    int min = Integer.MAX_VALUE, max = Integer.MIN_VALUE, i=0;
    double sum = 0;
    while(in.hasNext()) {
      int n = in.nextInt();
      min = Math.min(n, min);
      max = Math.max(n, max);
      sum += n;
      i++;
    }

    System.out.println("Min: " + min);
    System.out.println("Max: " + max);
    System.out.println("Avg: " + sum/i);

  }

  /*
  Exponential
  Min: 0
  Max: 3164
  Avg: 298.56693

  Pareto
  Min: 51
  Max: 99084
  Avg: 304.29187
  */
}
