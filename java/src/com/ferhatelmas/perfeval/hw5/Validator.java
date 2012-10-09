package com.ferhatelmas.perfeval.hw5;


import java.io.BufferedReader;
import java.io.FileReader;

public class Validator {

  public static void main(String[] args) throws Exception {
    BufferedReader br = new BufferedReader(new FileReader("01DEC2003-18APR2012.txt"));
    String line;
    int monthPrev = -1, dayPrev = -1;
    String prev = "";

    while((line=br.readLine()) != null) {
      String[] dates = line.split(" ")[0].split("/");
      int month = Integer.parseInt(dates[1]);
      int day = Integer.parseInt(dates[2]);
      if(month == monthPrev && dayPrev+1 != day) {
          System.out.println(prev + "\t-\t" + line);
      }
      monthPrev = month;
      dayPrev = day;
      prev = line;
    }
  }
}