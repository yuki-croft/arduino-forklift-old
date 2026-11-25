package org.junit;
public class Assert {
  public static void fail(String m){ throw new AssertionError(m); }
  public static void assertTrue(boolean c){ if(!c) fail("expected true"); }
  public static void assertTrue(String m, boolean c){ if(!c) fail(m); }
  public static void assertFalse(boolean c){ if(c) fail("expected false"); }
  public static void assertFalse(String m, boolean c){ if(c) fail(m); }
  public static void assertEquals(Object a, Object b){ if(!(a==null?b==null:a.equals(b))) fail("expected <"+a+"> but was <"+b+">"); }
  public static void assertEquals(String m, Object a, Object b){ if(!(a==null?b==null:a.equals(b))) fail(m+": expected <"+a+"> but was <"+b+">"); }
  public static void assertEquals(long a, long b){ if(a!=b) fail("expected <"+a+"> but was <"+b+">"); }
}
