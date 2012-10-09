package com.ferhatelmas.perfeval.mini.util;

public class Util {

    public static long nextPrime(long l) {
        while(!isPrime(l)) l++;
        return l;
    }

    private static boolean isPrime(long l) {
        for(long i=2; i<=Math.sqrt(l); i++) {
            if(l%i == 0) return false;
        }
        return true;
    }

}
