package com.ferhatelmas.perfeval.mini.run;

import com.ferhatelmas.perfeval.mini.util.Util;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Random;

public class SimpleGenerator {

    public static void main(String[] args) throws IOException {

        PrintWriter pw = new PrintWriter(new FileWriter("exponentialRandoms"));

        Random r = new Random(Util.nextPrime(System.currentTimeMillis()));
        for(int i=0; i<100000; i++) {
            pw.println(generateExponential(r.nextDouble()));
        }
        pw.close();
    }


    private static int generatePareto(double random) {
        return (int)Math.pow(-((random-1)*Math.pow(100000, 1.1) - random*Math.pow(51, 1.1)) /
                (Math.pow(100000*51, 1.1)), -1/1.1);
    }

    private static int generateExponential(double random) {
        return (int)(-Math.log(random) * 300);
    }

}
