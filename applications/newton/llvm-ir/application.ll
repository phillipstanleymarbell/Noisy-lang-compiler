; ModuleID = 'c-files/application.c'
source_filename = "c-files/application.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !7 {
  %1 = alloca i32, align 4
  %2 = alloca double, align 8
  %3 = alloca double, align 8
  %4 = alloca double, align 8
  %5 = alloca double, align 8
  %6 = alloca double, align 8
  store i32 0, i32* %1, align 4
  call void @llvm.dbg.declare(metadata double* %2, metadata !11, metadata !DIExpression()), !dbg !14
  store double 5.000000e+00, double* %2, align 8, !dbg !14
  call void @llvm.dbg.declare(metadata double* %3, metadata !15, metadata !DIExpression()), !dbg !16
  call void @llvm.dbg.declare(metadata double* %4, metadata !17, metadata !DIExpression()), !dbg !19
  store double 5.000000e+00, double* %4, align 8, !dbg !19
  call void @llvm.dbg.declare(metadata double* %5, metadata !20, metadata !DIExpression()), !dbg !22
  store double 5.000000e+00, double* %5, align 8, !dbg !22
  call void @llvm.dbg.declare(metadata double* %6, metadata !23, metadata !DIExpression()), !dbg !24
  %7 = load double, double* %2, align 8, !dbg !25
  %8 = load double, double* %2, align 8, !dbg !26
  %9 = fadd double %7, %8, !dbg !27
  store double %9, double* %3, align 8, !dbg !28
  %10 = load double, double* %2, align 8, !dbg !29
  %11 = load double, double* %2, align 8, !dbg !30
  %12 = fadd double %10, %11, !dbg !31
  store double %12, double* %3, align 8, !dbg !32
  %13 = load double, double* %3, align 8, !dbg !33
  store double %13, double* %6, align 8, !dbg !34
  ret i32 0, !dbg !35
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind optnone uwtable "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "c-files/application.c", directory: "/home/blackgeorge/Noisy-lang-compiler/applications/newton/llvm-ir")
!2 = !{}
!3 = !{i32 7, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11"}
!7 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 12, type: !8, scopeLine: 13, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!8 = !DISubroutineType(types: !9)
!9 = !{!10}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !DILocalVariable(name: "accelerationX", scope: !7, file: !1, line: 14, type: !12)
!12 = !DIDerivedType(tag: DW_TAG_typedef, name: "signalAccelerationX", file: !1, line: 7, baseType: !13)
!13 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!14 = !DILocation(line: 14, column: 23, scope: !7)
!15 = !DILocalVariable(name: "accelerationX2", scope: !7, file: !1, line: 14, type: !12)
!16 = !DILocation(line: 14, column: 44, scope: !7)
!17 = !DILocalVariable(name: "accelerationY", scope: !7, file: !1, line: 15, type: !18)
!18 = !DIDerivedType(tag: DW_TAG_typedef, name: "signalAccelerationY", file: !1, line: 8, baseType: !13)
!19 = !DILocation(line: 15, column: 23, scope: !7)
!20 = !DILocalVariable(name: "accelerationZ", scope: !7, file: !1, line: 16, type: !21)
!21 = !DIDerivedType(tag: DW_TAG_typedef, name: "signalAccelerationZ", file: !1, line: 9, baseType: !13)
!22 = !DILocation(line: 16, column: 23, scope: !7)
!23 = !DILocalVariable(name: "x", scope: !7, file: !1, line: 17, type: !13)
!24 = !DILocation(line: 17, column: 12, scope: !7)
!25 = !DILocation(line: 22, column: 19, scope: !7)
!26 = !DILocation(line: 22, column: 35, scope: !7)
!27 = !DILocation(line: 22, column: 33, scope: !7)
!28 = !DILocation(line: 22, column: 17, scope: !7)
!29 = !DILocation(line: 27, column: 19, scope: !7)
!30 = !DILocation(line: 27, column: 35, scope: !7)
!31 = !DILocation(line: 27, column: 33, scope: !7)
!32 = !DILocation(line: 27, column: 17, scope: !7)
!33 = !DILocation(line: 28, column: 9, scope: !7)
!34 = !DILocation(line: 28, column: 7, scope: !7)
!35 = !DILocation(line: 29, column: 2, scope: !7)
