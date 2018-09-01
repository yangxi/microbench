//print out our path

import org.vmmagic.unboxed.Address;
import org.vmmagic.unboxed.Word;
import org.vmmagic.unboxed.Offset;
import org.vmmagic.unboxed.ObjectReference;
import org.jikesrvm.scheduler.RVMThread;
import org.jikesrvm.compilers.common.CompiledMethods;
import org.jikesrvm.classloader.Atom;

public class readme{
  public static void main(String[] args){
    System.out.println("hello world");
    RVMThread thr = null;
    for(int i=0; i< RVMThread.threadBySlot.length; i++){
      if (RVMThread.threadBySlot[i] != null){
	thr = RVMThread.threadBySlot[i];
	reportTracks(thr);
      }
    }
  }

  public static void reportTracks(RVMThread my){

    Address ftlbheader = ObjectReference.fromObject(my).toAddress().plus(org.jikesrvm.compilers.common.assembler.ia32.Assembler.ftlbHeaderOffset);
    Address ftlbbuffer = ObjectReference.fromObject(my).toAddress().plus(org.jikesrvm.compilers.common.assembler.ia32.Assembler.ftlbBufOffset);
    int tlbsize = (org.jikesrvm.compilers.common.assembler.ia32.Assembler.ftlbBufMask + 1);
    String myname  = my.getName();
    int nr_store = ftlbheader.loadInt();
    int offset = nr_store % tlbsize;
    Address curr_offset = ftlbbuffer.plus(nr_store % tlbsize);

    System.out.println(myname + " store " + nr_store + " data, point to " + offset);

    if (nr_store < 11)
      return;

    for (int i=0; i<10; i++){
      curr_offset = curr_offset.minus(8);
      if (curr_offset.LT(ftlbbuffer)){
	curr_offset = ftlbbuffer.plus(tlbsize);
	System.out.println("rounding");
      }

      int timestamp = curr_offset.loadInt();
      int cmid = curr_offset.plus(4).loadInt();
      if (cmid > 0){
	Atom mname = CompiledMethods.getCompiledMethodUnchecked(cmid).method.getName();
	System.out.println(i + " : " + timestamp + " enter " + cmid + " " + mname);
      } else {
	Atom mname = CompiledMethods.getCompiledMethodUnchecked(-cmid).method.getName();
	System.out.println(i + " : " + timestamp + " leave " + cmid + " " + mname);
      }
    }

  //   int i = 0;
  //   while(true){
  //     if (cmid == 0){
  // 	System.out.println("Thread " + cmid + "is empty");
  // 	return;
  //     }

  //     if (cmid > 0){
  // 	String start = " call ";
  // 	Atom mname = CompiledMethods.getCompiledMethodUnchecked(cmid).method.getName();
  // 	System.out.println("Thread " + myname + start + cmid + " " + mname);
  //     } else {
  // 	String start = " ret ";
  // 	Atom mname = CompiledMethods.getCompiledMethodUnchecked(-cmid).method.getName();
  // 	System.out.println("Thread " + myname + start + cmid + " " + mname);
  //     }

  //     i = i+4;
  //     if (i >= tlbsize){
  // 	i = 0;
  // 	if (ftlbbase.loadInt() == -1){
  // 	  System.out.println("We catch up with thread now");
  // 	  return;
  // 	}
  //     }
  //     cmid = ftlbbase.plus(i).loadInt();
  //   }
  // }
	}
}
