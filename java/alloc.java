import probe.MMTkProbe;
import java.lang.System;
import probe.PerfEventLauncherProbe;


class alloc {
    static int[] str;
    static int[] dst;
    static volatile long sum;
    public static void main(String[] args) {
	MMTkProbe probe = new MMTkProbe();
	PerfEventLauncherProbe perf = new PerfEventLauncherProbe();
	probe.init();
	perf.init();
	boolean warmup;
	String benchname = args[0];
	boolean initall = false;
	boolean copy = false;

	if(args[0].compareTo("noinit")==0)
	    initall = false;
	else if(args[0].compareTo("initall")==0)
	    initall = true;
	else if(args[0].compareTo("initcopy")==0){
	    initall = true; copy = true;
	}
	else if(args[0].compareTo("copy") == 0){
	    copy = true;
	}
	
	for(int iteration=0;iteration<5; iteration++){

	    if (iteration < 4){
		System.out.println("===== Alloc "+benchname+" starting warmup =====");
		warmup = true;
	    }
	    else{
		System.out.println("===== alloc "+benchname+" starting =====");
		warmup = false;
	    }

	    long beginTime = System.currentTimeMillis();
	    
	    perf.begin("alloc", iteration, warmup);
	    probe.begin("alloc", iteration, warmup);
	    kernel(initall,copy);
	    probe.end("alloc", iteration, warmup);    	    
	    perf.end("alloc", iteration, warmup);    	    

	    long execTime = System.currentTimeMillis() - beginTime;

	    if (iteration < 4)
		System.out.println("===== Alloc "+ benchname+" completed warmup in " + execTime + " msec =====");
	    else
		System.out.println("===== Alloc "+ benchname+" PASSED in " + execTime + " msec =====");       
	}
    }

    public static void kernel(boolean initall, boolean copy) {
	int k = 0;
	if (copy){
	    dst = new int[8*1024];
	}
	

	for(int i=0;i<1024*1024*64;i++){
		sum =0;
		str= new int[16];
		
		if(initall)
		    for (int j=0;j<16;j++)
			str[j] = j;
		       
		if(copy)
		    for (int j=0;j<16;j++,k++)
			dst[k%(8*1024)] = str[j];	   
	}
    }
}