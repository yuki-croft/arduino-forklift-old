import java.lang.reflect.*;
public class RunTests {
  public static void main(String[] a) throws Exception {
    int pass=0, fail=0;
    for (String cn : a) {
      Class<?> c = Class.forName(cn);
      for (Method m : c.getDeclaredMethods()) {
        if (m.getAnnotation(org.junit.Test.class)==null) continue;
        try { m.invoke(c.getDeclaredConstructor().newInstance()); pass++; System.out.println("PASS  "+c.getSimpleName()+"."+m.getName()); }
        catch (InvocationTargetException e) { fail++; System.out.println("FAIL  "+c.getSimpleName()+"."+m.getName()+"  -> "+e.getCause()); }
      }
    }
    System.out.println("pass="+pass+" fail="+fail);
    System.exit(fail==0?0:1);
  }
}
