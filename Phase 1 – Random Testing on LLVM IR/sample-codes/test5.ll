; ModuleID = 'test5.c'
source_filename = "test5.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %var1 = alloca i32, align 4
  %var2 = alloca i32, align 4
  %var3 = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  store i32 0, i32* %var3, align 4
  %0 = load i32, i32* %var1, align 4
  %cmp = icmp eq i32 %0, 5
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %1 = load i32, i32* %var3, align 4
  %sub = sub nsw i32 %1, 5
  store i32 %sub, i32* %var3, align 4
  br label %if.end4

if.else:                                          ; preds = %entry
  %2 = load i32, i32* %var1, align 4
  %cmp1 = icmp eq i32 %2, 10
  br i1 %cmp1, label %if.then2, label %if.else3

if.then2:                                         ; preds = %if.else
  store i32 12, i32* %var3, align 4
  br label %if.end

if.else3:                                         ; preds = %if.else
  %3 = load i32, i32* %var3, align 4
  %add = add nsw i32 %3, 1
  store i32 %add, i32* %var3, align 4
  br label %if.end

if.end:                                           ; preds = %if.else3, %if.then2
  br label %if.end4

if.end4:                                          ; preds = %if.end, %if.then
  %4 = load i32, i32* %var2, align 4
  %cmp5 = icmp ne i32 %4, -8
  br i1 %cmp5, label %if.then6, label %if.else8

if.then6:                                         ; preds = %if.end4
  %5 = load i32, i32* %var3, align 4
  %sub7 = sub nsw i32 %5, 3
  store i32 %sub7, i32* %var3, align 4
  br label %if.end9

if.else8:                                         ; preds = %if.end4
  %6 = load i32, i32* %var3, align 4
  %mul = mul nsw i32 %6, 2
  store i32 %mul, i32* %var3, align 4
  br label %if.end9

if.end9:                                          ; preds = %if.else8, %if.then6
  ret i32 0
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1~18.04.2 "}
