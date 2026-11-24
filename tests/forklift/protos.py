import re,sys
out=[]
for f in sys.argv[1:]:
    for line in open(f,encoding="utf-8"):
        m=re.match(r'^((?:const\s+)?[A-Za-z_][\w\s\*&:<>]*?\b\w+\s*\([^;{]*\))\s*\{',line)
        if m and not line.startswith(("if","for","while","switch")): out.append(m.group(1)+";")
print("\n".join(sorted(set(out))))
