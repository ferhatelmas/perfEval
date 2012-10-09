package com.ferhatelmas.perfeval.hw1;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DataCollecter {

  private static Pattern pattern = Pattern.compile("\\s*<td>(\\d+\\.\\d*)\\s*</td>\\s*");

  public static void main(String[] args) throws Exception {
    run1();
    run2();
    run34();
  }

  private static void run34() throws Exception {
    PrintWriter pw = new PrintWriter(new FileOutputStream("runs.txt"));

    for(int i=0; i<1000; i++) {
      for(int clients=1; clients<=1000; clients++) {
        for(int aps=1; aps<=10; aps++) {
          for(int servers=1; servers<=10; servers++) {
            pw.write(clients + "\t" + aps + "\t" + servers + "\t");
            writeDoubleArray(pw, getRunResult(clients, aps, servers));
          }
        }
      }
    }
    pw.close();
  }

  private static void run2() throws Exception {
    PrintWriter pw = new PrintWriter(new FileOutputStream("run2.txt"));
    for(int i=1; i<=1000; i++) {
      pw.write(i + "\t");
      writeDoubleArray(pw, getRunResult(i, 1, 1));
    }
    pw.close();
  }

  private static void run1() throws Exception {
    PrintWriter pw = new PrintWriter(new FileOutputStream("run1.txt"));
    for(int i=0; i<1000; i++) {
      writeDoubleArray(pw, getRunResult(1, 1, 1));
    }
    pw.close();
  }

  private static void writeDoubleArray(PrintWriter pw, double[] result) {
    for(double d : result) pw.write(d + "\t");
    pw.write("\n");
  }

  private static double[] getRunResult(int clients, int aps, int servers) throws Exception {
    URL url = new URL("http://128.178.151.40/output.php");
    HttpURLConnection conn = (HttpURLConnection)url.openConnection();
    conn.setRequestMethod("POST");
    conn.setDoOutput(true);

    String pushData = URLEncoder.encode("clients", "UTF-8") + "=" + 
      URLEncoder.encode(String.valueOf(clients), "UTF-8") + "&" + 
      URLEncoder.encode("apoints", "UTF-8") + "=" + 
      URLEncoder.encode(String.valueOf(aps), "UTF-8") + "&" + 
      URLEncoder.encode("servers", "UTF-8") + "=" + 
      URLEncoder.encode(String.valueOf(servers), "UTF-8");

    OutputStreamWriter osw = new OutputStreamWriter(conn.getOutputStream());
    osw.write(pushData);
    osw.close();

    conn.connect();
    double[] result = parseResponse(conn.getInputStream());
    conn.disconnect();
    return result;
  }

  private static double[] parseResponse(InputStream in) throws Exception {
    BufferedReader br = new BufferedReader(new InputStreamReader(in));

    double[] result = new double[4];
    int index = 0;

    String line;
    while((line=br.readLine()) != null) {
      Matcher m = pattern.matcher(line);
      if(m.matches()) {
        result[index++] = Double.parseDouble(m.group(1));
      }
    }
    return result;
  }
}