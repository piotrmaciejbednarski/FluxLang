; ModuleID = "flux_module"
target triple = "x86_64-pc-linux-gnu"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

define i32 @"main"()
{
entry:
  %"x" = alloca i32
  store i32 0, i32* %"x"
  br label %"while.cond"
while.cond:
  %"x.1" = load i32, i32* %"x"
  %".4" = icmp sge i32 %"x.1", 1000000000
  br i1 %".4", label %"while.body", label %"while.end"
while.body:
  %"x.2" = load i32, i32* %"x"
  %".6" = add i32 %"x.2", 1
  br label %"while.cond"
while.end:
  ret i32 0
}

