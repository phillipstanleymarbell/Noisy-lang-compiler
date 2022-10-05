; ModuleID = 'main_output.bc'
source_filename = "main.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, %struct._IO_codecvt*, %struct._IO_wide_data*, %struct._IO_FILE*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type opaque
%struct._IO_codecvt = type opaque
%struct._IO_wide_data = type opaque
%struct.timeval = type { i64, i64 }
%struct.timezone = type { i32, i32 }

@.str = private unnamed_addr constant [2 x i8] c"r\00", align 1
@.str.1 = private unnamed_addr constant [17 x i8] c"%G,%G,%G,%G,%*G\0A\00", align 1
@stderr = external dso_local global %struct._IO_FILE*, align 8
@.str.2 = private unnamed_addr constant [62 x i8] c"Unable to read row. Possible problem with input data format.\0A\00", align 1
@.str.3 = private unnamed_addr constant [5 x i8] c"%lf\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 %0, i8** %1) #0 !dbg !7 {
  %3 = alloca [4 x float], align 16
  %4 = alloca [4 x float], align 16
  %5 = alloca [4 x float], align 16
  %6 = alloca [4 x float], align 16
  %7 = alloca %struct.timeval, align 8
  call void @llvm.dbg.value(metadata i32 %0, metadata !14, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i8** %1, metadata !16, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.declare(metadata [150 x i8]* undef, metadata !17, metadata !DIExpression()), !dbg !21
  %8 = getelementptr inbounds i8*, i8** %1, i64 1, !dbg !22
  %9 = load i8*, i8** %8, align 8, !dbg !22
  %10 = call %struct._IO_FILE* @fopen(i8* %9, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i64 0, i64 0)), !dbg !23
  call void @llvm.dbg.value(metadata %struct._IO_FILE* %10, metadata !24, metadata !DIExpression()), !dbg !15
  br label %11, !dbg !85

11:                                               ; preds = %11, %2
  %12 = call i32 @fgetc(%struct._IO_FILE* %10), !dbg !86
  %13 = icmp ne i32 %12, 10, !dbg !87
  br i1 %13, label %11, label %14, !dbg !85, !llvm.loop !88

14:                                               ; preds = %11
  call void @llvm.dbg.value(metadata i32 0, metadata !91, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 -1, metadata !92, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 0, metadata !93, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 1, metadata !94, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 0, metadata !95, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 0, metadata !96, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 1, metadata !97, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !98, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.declare(metadata float* undef, metadata !100, metadata !DIExpression()), !dbg !101
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !102, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !103, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !104, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float -1.000000e+03, metadata !105, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.declare(metadata float* undef, metadata !106, metadata !DIExpression()), !dbg !107
  call void @llvm.dbg.declare(metadata [4 x float]* %3, metadata !108, metadata !DIExpression()), !dbg !112
  call void @llvm.dbg.declare(metadata [4 x float]* %4, metadata !113, metadata !DIExpression()), !dbg !114
  call void @llvm.dbg.declare(metadata [4 x float]* %5, metadata !115, metadata !DIExpression()), !dbg !116
  call void @llvm.dbg.declare(metadata [4 x float]* %6, metadata !117, metadata !DIExpression()), !dbg !118
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !119, metadata !DIExpression()), !dbg !123
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !124, metadata !DIExpression()), !dbg !125
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !126, metadata !DIExpression()), !dbg !127
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !128, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !129, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !130, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !131, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !132, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !133, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !134, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !135, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !136, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !137, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !139, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !141, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !143, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !144, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !145, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !146, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !147, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !148, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.declare(metadata %struct.timeval* %7, metadata !149, metadata !DIExpression()), !dbg !157
  %15 = call i32 @gettimeofday(%struct.timeval* %7, %struct.timezone* null) #4, !dbg !158
  %16 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 0, !dbg !159
  %17 = load i64, i64* %16, align 8, !dbg !159
  %18 = sitofp i64 %17 to double, !dbg !160
  %19 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 1, !dbg !161
  %20 = load i64, i64* %19, align 8, !dbg !161
  %21 = sitofp i64 %20 to double, !dbg !162
  %22 = fmul double 0x3EB0C6F7A0B5ED8D, %21, !dbg !163
  %23 = fadd double %18, %22, !dbg !164
  call void @llvm.dbg.value(metadata double %23, metadata !165, metadata !DIExpression()), !dbg !15
  br label %24, !dbg !167

24:                                               ; preds = %80, %53, %14
  %.026 = phi float [ -1.000000e+03, %14 ], [ %.127, %80 ], [ %.026, %53 ], !dbg !15
  %.024 = phi float [ undef, %14 ], [ %.125, %80 ], [ %.125, %53 ]
  %.019 = phi float [ -1.000000e+04, %14 ], [ %.221, %80 ], [ %.019, %53 ], !dbg !15
  %.016 = phi float [ 1.000000e+04, %14 ], [ %.218, %80 ], [ %.016, %53 ], !dbg !15
  %.014 = phi float [ 0.000000e+00, %14 ], [ %.115, %80 ], [ %.014, %53 ], !dbg !15
  %.012 = phi i32 [ 0, %14 ], [ %.113, %80 ], [ %.012, %53 ], !dbg !15
  %.010 = phi i32 [ 0, %14 ], [ %.111, %80 ], [ %.010, %53 ], !dbg !15
  %.08 = phi i32 [ 1, %14 ], [ %.2, %80 ], [ %.08, %53 ], !dbg !15
  %.06 = phi i32 [ -1, %14 ], [ %85, %80 ], [ %spec.select, %53 ], !dbg !15
  %.04 = phi i32 [ 0, %14 ], [ %.15, %80 ], [ %.15, %53 ], !dbg !15
  %.03 = phi float [ 0.000000e+00, %14 ], [ %.023, %80 ], [ %spec.select1, %53 ], !dbg !15
  call void @llvm.dbg.value(metadata float %.03, metadata !145, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %.04, metadata !91, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %.06, metadata !92, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %.08, metadata !94, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %.010, metadata !95, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %.012, metadata !96, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.014, metadata !136, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.016, metadata !133, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.019, metadata !130, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.024, metadata !168, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.026, metadata !105, metadata !DIExpression()), !dbg !15
  %25 = icmp ne i32 %.04, 0, !dbg !169
  %26 = xor i1 %25, true, !dbg !169
  br i1 %26, label %27, label %86, !dbg !167

27:                                               ; preds = %38, %24
  %.028 = phi float [ %43, %38 ], [ 0.000000e+00, %24 ], !dbg !170
  %.125 = phi float [ %51, %38 ], [ %.024, %24 ]
  %.023 = phi float [ %46, %38 ], [ 0.000000e+00, %24 ], !dbg !170
  %.022 = phi float [ %49, %38 ], [ 0.000000e+00, %24 ], !dbg !170
  %.02 = phi float [ %39, %38 ], [ 0.000000e+00, %24 ], !dbg !170
  %.01 = phi i32 [ %40, %38 ], [ 0, %24 ], !dbg !170
  %.0 = phi i64 [ %52, %38 ], [ 0, %24 ], !dbg !172
  call void @llvm.dbg.value(metadata i64 %.0, metadata !174, metadata !DIExpression()), !dbg !172
  call void @llvm.dbg.value(metadata i32 %.01, metadata !175, metadata !DIExpression()), !dbg !170
  call void @llvm.dbg.value(metadata float %.02, metadata !176, metadata !DIExpression()), !dbg !170
  call void @llvm.dbg.value(metadata float %.022, metadata !177, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.023, metadata !178, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.125, metadata !168, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.028, metadata !179, metadata !DIExpression()), !dbg !15
  %28 = icmp ult i64 %.0, 4, !dbg !180
  br i1 %28, label %29, label %53, !dbg !182

29:                                               ; preds = %27
  %30 = getelementptr inbounds [4 x float], [4 x float]* %3, i64 0, i64 %.0, !dbg !183
  %31 = getelementptr inbounds [4 x float], [4 x float]* %4, i64 0, i64 %.0, !dbg !185
  %32 = getelementptr inbounds [4 x float], [4 x float]* %5, i64 0, i64 %.0, !dbg !186
  %33 = getelementptr inbounds [4 x float], [4 x float]* %6, i64 0, i64 %.0, !dbg !187
  %34 = call i32 (%struct._IO_FILE*, i8*, ...) @__isoc99_fscanf(%struct._IO_FILE* %10, i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.1, i64 0, i64 0), float* %30, float* %31, float* %32, float* %33), !dbg !188
  call void @llvm.dbg.value(metadata i32 %34, metadata !189, metadata !DIExpression()), !dbg !190
  switch i32 %34, label %35 [
    i32 -1, label %53
    i32 0, label %53
    i32 4, label %38
  ], !dbg !191

35:                                               ; preds = %29
  %36 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 8, !dbg !193
  %37 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %36, i8* getelementptr inbounds ([62 x i8], [62 x i8]* @.str.2, i64 0, i64 0)), !dbg !196
  br label %38, !dbg !197

38:                                               ; preds = %35, %29
  %39 = fadd float %.02, %.125, !dbg !198
  call void @llvm.dbg.value(metadata float %39, metadata !176, metadata !DIExpression()), !dbg !170
  %40 = add nsw i32 %.01, 1, !dbg !199
  call void @llvm.dbg.value(metadata i32 %40, metadata !175, metadata !DIExpression()), !dbg !170
  %41 = getelementptr inbounds [4 x float], [4 x float]* %4, i64 0, i64 %.0, !dbg !200
  %42 = load float, float* %41, align 4, !dbg !200
  %43 = fadd float %.028, %42, !dbg !201
  call void @llvm.dbg.value(metadata float %43, metadata !179, metadata !DIExpression()), !dbg !15
  %44 = getelementptr inbounds [4 x float], [4 x float]* %5, i64 0, i64 %.0, !dbg !202
  %45 = load float, float* %44, align 4, !dbg !202
  %46 = fadd float %.023, %45, !dbg !203
  call void @llvm.dbg.value(metadata float %46, metadata !178, metadata !DIExpression()), !dbg !15
  %47 = getelementptr inbounds [4 x float], [4 x float]* %6, i64 0, i64 %.0, !dbg !204
  %48 = load float, float* %47, align 4, !dbg !204
  %49 = fadd float %.022, %48, !dbg !205
  call void @llvm.dbg.value(metadata float %49, metadata !177, metadata !DIExpression()), !dbg !15
  %50 = getelementptr inbounds [4 x float], [4 x float]* %3, i64 0, i64 %.0, !dbg !206
  %51 = load float, float* %50, align 4, !dbg !206
  call void @llvm.dbg.value(metadata float %51, metadata !168, metadata !DIExpression()), !dbg !15
  %52 = add i64 %.0, 1, !dbg !207
  call void @llvm.dbg.value(metadata i64 %52, metadata !174, metadata !DIExpression()), !dbg !172
  br label %27, !dbg !208, !llvm.loop !209

53:                                               ; preds = %29, %29, %27
  %.15 = phi i32 [ %.04, %27 ], [ 1, %29 ], [ 1, %29 ], !dbg !15
  call void @llvm.dbg.value(metadata i32 %.15, metadata !91, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.declare(metadata float* undef, metadata !211, metadata !DIExpression()), !dbg !212
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !137, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !139, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.03, metadata !141, metadata !DIExpression()), !dbg !15
  %54 = icmp eq i32 %.06, -1, !dbg !213
  %spec.select = select i1 %54, i32 0, i32 %.06, !dbg !215
  %spec.select1 = select i1 %54, float %.023, float %.03, !dbg !215
  call void @llvm.dbg.value(metadata float %spec.select1, metadata !145, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %spec.select, metadata !92, metadata !DIExpression()), !dbg !15
  %55 = fsub float %.023, %spec.select1, !dbg !216
  %56 = call float @llvm.fabs.f32(float %55), !dbg !218
  %57 = fpext float %56 to double, !dbg !218
  %58 = fcmp ogt double %57, 5.000000e-01, !dbg !219
  br i1 %58, label %59, label %24, !dbg !220, !llvm.loop !221

59:                                               ; preds = %53
  call void @llvm.dbg.value(metadata float %.023, metadata !145, metadata !DIExpression()), !dbg !15
  %60 = fcmp ogt float %.023, %.019, !dbg !223
  %.120 = select i1 %60, float %.023, float %.019, !dbg !225
  call void @llvm.dbg.value(metadata float %.120, metadata !130, metadata !DIExpression()), !dbg !15
  %61 = fcmp ogt float %.016, %.023, !dbg !226
  %.117 = select i1 %61, float %.023, float %.016, !dbg !228
  call void @llvm.dbg.value(metadata float %.117, metadata !133, metadata !DIExpression()), !dbg !15
  %62 = fsub float %.120, %.117, !dbg !229
  call void @llvm.dbg.value(metadata float %62, metadata !230, metadata !DIExpression()), !dbg !15
  %63 = icmp slt i32 %spec.select, 10, !dbg !231
  %64 = srem i32 %spec.select, 50
  %65 = icmp ne i32 %64, 0
  %or.cond = or i1 %63, %65, !dbg !233
  br i1 %or.cond, label %66, label %69, !dbg !233

66:                                               ; preds = %59
  %67 = fadd float %.120, %.117, !dbg !234
  %68 = fdiv float %67, 2.000000e+00, !dbg !236
  call void @llvm.dbg.value(metadata float %68, metadata !136, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.120, metadata !237, metadata !DIExpression()), !dbg !238
  call void @llvm.dbg.value(metadata float %.117, metadata !130, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.120, metadata !133, metadata !DIExpression()), !dbg !15
  br label %69, !dbg !239

69:                                               ; preds = %66, %59
  %.221 = phi float [ %.117, %66 ], [ %.120, %59 ], !dbg !170
  %.218 = phi float [ %.120, %66 ], [ %.117, %59 ], !dbg !170
  %.115 = phi float [ %68, %66 ], [ %.014, %59 ], !dbg !15
  call void @llvm.dbg.value(metadata float %.115, metadata !136, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.218, metadata !133, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.221, metadata !130, metadata !DIExpression()), !dbg !15
  %70 = fcmp ogt float %.03, %.023, !dbg !240
  %71 = fcmp ogt float %.115, %.023
  %or.cond3 = and i1 %70, %71, !dbg !242
  br i1 %or.cond3, label %72, label %80, !dbg !242

72:                                               ; preds = %69
  %73 = fsub float %.125, %.026, !dbg !243
  %74 = fpext float %73 to double, !dbg !244
  %75 = fcmp ogt double %74, 3.000000e-01, !dbg !245
  %76 = fcmp ogt float %62, 2.000000e+00
  %or.cond4 = and i1 %75, %76, !dbg !246
  %77 = fcmp olt float %62, 8.000000e+00
  %or.cond5 = and i1 %or.cond4, %77, !dbg !246
  br i1 %or.cond5, label %78, label %80, !dbg !246

78:                                               ; preds = %72
  %79 = add nsw i32 %.010, 1, !dbg !247
  call void @llvm.dbg.value(metadata i32 %79, metadata !95, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 0, metadata !94, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.125, metadata !105, metadata !DIExpression()), !dbg !15
  br label %80, !dbg !249

80:                                               ; preds = %78, %72, %69
  %.127 = phi float [ %.125, %78 ], [ %.026, %72 ], [ %.026, %69 ], !dbg !15
  %.111 = phi i32 [ %79, %78 ], [ %.010, %72 ], [ %.010, %69 ], !dbg !15
  %.19 = phi i32 [ 0, %78 ], [ %.08, %72 ], [ %.08, %69 ], !dbg !15
  call void @llvm.dbg.value(metadata i32 %.19, metadata !94, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %.111, metadata !95, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata float %.127, metadata !105, metadata !DIExpression()), !dbg !15
  %81 = fsub float %.125, %.127, !dbg !250
  %82 = fcmp ogt float %81, 2.000000e+00, !dbg !252
  %83 = icmp eq i32 %.19, 0
  %or.cond6 = and i1 %82, %83, !dbg !253
  %84 = add nsw i32 %.012, 1, !dbg !253
  %.113 = select i1 %or.cond6, i32 %84, i32 %.012, !dbg !253
  %.2 = select i1 %or.cond6, i32 1, i32 %.19, !dbg !253
  call void @llvm.dbg.value(metadata i32 %.2, metadata !94, metadata !DIExpression()), !dbg !15
  call void @llvm.dbg.value(metadata i32 %.113, metadata !96, metadata !DIExpression()), !dbg !15
  %85 = add nsw i32 %spec.select, 1, !dbg !254
  call void @llvm.dbg.value(metadata i32 %85, metadata !92, metadata !DIExpression()), !dbg !15
  br label %24, !dbg !167, !llvm.loop !221

86:                                               ; preds = %24
  %87 = call i32 @gettimeofday(%struct.timeval* %7, %struct.timezone* null) #4, !dbg !255
  %88 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 0, !dbg !256
  %89 = load i64, i64* %88, align 8, !dbg !256
  %90 = sitofp i64 %89 to double, !dbg !257
  %91 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 1, !dbg !258
  %92 = load i64, i64* %91, align 8, !dbg !258
  %93 = sitofp i64 %92 to double, !dbg !259
  %94 = fmul double 0x3EB0C6F7A0B5ED8D, %93, !dbg !260
  %95 = fadd double %90, %94, !dbg !261
  call void @llvm.dbg.value(metadata double %95, metadata !262, metadata !DIExpression()), !dbg !15
  %96 = fsub double %95, %23, !dbg !263
  %97 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.3, i64 0, i64 0), double %96), !dbg !264
  %98 = call i32 @fclose(%struct._IO_FILE* %10), !dbg !265
  ret i32 0, !dbg !266
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare dso_local %struct._IO_FILE* @fopen(i8*, i8*) #2

declare dso_local i32 @fgetc(%struct._IO_FILE*) #2

; Function Attrs: nounwind
declare dso_local i32 @gettimeofday(%struct.timeval*, %struct.timezone*) #3

declare dso_local i32 @__isoc99_fscanf(%struct._IO_FILE*, i8*, ...) #2

declare dso_local i32 @fprintf(%struct._IO_FILE*, i8*, ...) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.fabs.f32(float) #1

declare dso_local i32 @printf(i8*, ...) #2

declare dso_local i32 @fclose(%struct._IO_FILE*) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "main.c", directory: "/home/blackgeorge/Noisy-lang-compiler/applications/newton/llvm-ir/pedometer")
!2 = !{}
!3 = !{i32 7, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11"}
!7 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 50, type: !8, scopeLine: 51, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !10, !11}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!12 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !13, size: 64)
!13 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!14 = !DILocalVariable(name: "argc", arg: 1, scope: !7, file: !1, line: 50, type: !10)
!15 = !DILocation(line: 0, scope: !7)
!16 = !DILocalVariable(name: "argv", arg: 2, scope: !7, file: !1, line: 50, type: !11)
!17 = !DILocalVariable(name: "charBuffer", scope: !7, file: !1, line: 52, type: !18)
!18 = !DICompositeType(tag: DW_TAG_array_type, baseType: !13, size: 1200, elements: !19)
!19 = !{!20}
!20 = !DISubrange(count: 150)
!21 = !DILocation(line: 52, column: 7, scope: !7)
!22 = !DILocation(line: 57, column: 20, scope: !7)
!23 = !DILocation(line: 57, column: 14, scope: !7)
!24 = !DILocalVariable(name: "inputFile", scope: !7, file: !1, line: 53, type: !25)
!25 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !26, size: 64)
!26 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !27, line: 7, baseType: !28)
!27 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/FILE.h", directory: "")
!28 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_FILE", file: !29, line: 49, size: 1728, elements: !30)
!29 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h", directory: "")
!30 = !{!31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !46, !48, !49, !50, !54, !56, !58, !62, !65, !67, !70, !73, !74, !76, !80, !81}
!31 = !DIDerivedType(tag: DW_TAG_member, name: "_flags", scope: !28, file: !29, line: 51, baseType: !10, size: 32)
!32 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_ptr", scope: !28, file: !29, line: 54, baseType: !12, size: 64, offset: 64)
!33 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_end", scope: !28, file: !29, line: 55, baseType: !12, size: 64, offset: 128)
!34 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_base", scope: !28, file: !29, line: 56, baseType: !12, size: 64, offset: 192)
!35 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_base", scope: !28, file: !29, line: 57, baseType: !12, size: 64, offset: 256)
!36 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_ptr", scope: !28, file: !29, line: 58, baseType: !12, size: 64, offset: 320)
!37 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_end", scope: !28, file: !29, line: 59, baseType: !12, size: 64, offset: 384)
!38 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_base", scope: !28, file: !29, line: 60, baseType: !12, size: 64, offset: 448)
!39 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_end", scope: !28, file: !29, line: 61, baseType: !12, size: 64, offset: 512)
!40 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_base", scope: !28, file: !29, line: 64, baseType: !12, size: 64, offset: 576)
!41 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_backup_base", scope: !28, file: !29, line: 65, baseType: !12, size: 64, offset: 640)
!42 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_end", scope: !28, file: !29, line: 66, baseType: !12, size: 64, offset: 704)
!43 = !DIDerivedType(tag: DW_TAG_member, name: "_markers", scope: !28, file: !29, line: 68, baseType: !44, size: 64, offset: 768)
!44 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !45, size: 64)
!45 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_marker", file: !29, line: 36, flags: DIFlagFwdDecl)
!46 = !DIDerivedType(tag: DW_TAG_member, name: "_chain", scope: !28, file: !29, line: 70, baseType: !47, size: 64, offset: 832)
!47 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !28, size: 64)
!48 = !DIDerivedType(tag: DW_TAG_member, name: "_fileno", scope: !28, file: !29, line: 72, baseType: !10, size: 32, offset: 896)
!49 = !DIDerivedType(tag: DW_TAG_member, name: "_flags2", scope: !28, file: !29, line: 73, baseType: !10, size: 32, offset: 928)
!50 = !DIDerivedType(tag: DW_TAG_member, name: "_old_offset", scope: !28, file: !29, line: 74, baseType: !51, size: 64, offset: 960)
!51 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off_t", file: !52, line: 152, baseType: !53)
!52 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types.h", directory: "")
!53 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!54 = !DIDerivedType(tag: DW_TAG_member, name: "_cur_column", scope: !28, file: !29, line: 77, baseType: !55, size: 16, offset: 1024)
!55 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!56 = !DIDerivedType(tag: DW_TAG_member, name: "_vtable_offset", scope: !28, file: !29, line: 78, baseType: !57, size: 8, offset: 1040)
!57 = !DIBasicType(name: "signed char", size: 8, encoding: DW_ATE_signed_char)
!58 = !DIDerivedType(tag: DW_TAG_member, name: "_shortbuf", scope: !28, file: !29, line: 79, baseType: !59, size: 8, offset: 1048)
!59 = !DICompositeType(tag: DW_TAG_array_type, baseType: !13, size: 8, elements: !60)
!60 = !{!61}
!61 = !DISubrange(count: 1)
!62 = !DIDerivedType(tag: DW_TAG_member, name: "_lock", scope: !28, file: !29, line: 81, baseType: !63, size: 64, offset: 1088)
!63 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !64, size: 64)
!64 = !DIDerivedType(tag: DW_TAG_typedef, name: "_IO_lock_t", file: !29, line: 43, baseType: null)
!65 = !DIDerivedType(tag: DW_TAG_member, name: "_offset", scope: !28, file: !29, line: 89, baseType: !66, size: 64, offset: 1152)
!66 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off64_t", file: !52, line: 153, baseType: !53)
!67 = !DIDerivedType(tag: DW_TAG_member, name: "_codecvt", scope: !28, file: !29, line: 91, baseType: !68, size: 64, offset: 1216)
!68 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !69, size: 64)
!69 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_codecvt", file: !29, line: 37, flags: DIFlagFwdDecl)
!70 = !DIDerivedType(tag: DW_TAG_member, name: "_wide_data", scope: !28, file: !29, line: 92, baseType: !71, size: 64, offset: 1280)
!71 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !72, size: 64)
!72 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_wide_data", file: !29, line: 38, flags: DIFlagFwdDecl)
!73 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_list", scope: !28, file: !29, line: 93, baseType: !47, size: 64, offset: 1344)
!74 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_buf", scope: !28, file: !29, line: 94, baseType: !75, size: 64, offset: 1408)
!75 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!76 = !DIDerivedType(tag: DW_TAG_member, name: "__pad5", scope: !28, file: !29, line: 95, baseType: !77, size: 64, offset: 1472)
!77 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !78, line: 46, baseType: !79)
!78 = !DIFile(filename: "/usr/lib/llvm-12/lib/clang/12.0.1/include/stddef.h", directory: "")
!79 = !DIBasicType(name: "long unsigned int", size: 64, encoding: DW_ATE_unsigned)
!80 = !DIDerivedType(tag: DW_TAG_member, name: "_mode", scope: !28, file: !29, line: 96, baseType: !10, size: 32, offset: 1536)
!81 = !DIDerivedType(tag: DW_TAG_member, name: "_unused2", scope: !28, file: !29, line: 98, baseType: !82, size: 160, offset: 1568)
!82 = !DICompositeType(tag: DW_TAG_array_type, baseType: !13, size: 160, elements: !83)
!83 = !{!84}
!84 = !DISubrange(count: 20)
!85 = !DILocation(line: 67, column: 2, scope: !7)
!86 = !DILocation(line: 67, column: 9, scope: !7)
!87 = !DILocation(line: 67, column: 26, scope: !7)
!88 = distinct !{!88, !85, !89, !90}
!89 = !DILocation(line: 67, column: 34, scope: !7)
!90 = !{!"llvm.loop.mustprogress"}
!91 = !DILocalVariable(name: "reachedEOF", scope: !7, file: !1, line: 70, type: !10)
!92 = !DILocalVariable(name: "measurementCount", scope: !7, file: !1, line: 72, type: !10)
!93 = !DILocalVariable(name: "DCEstimateCounter", scope: !7, file: !1, line: 73, type: !10)
!94 = !DILocalVariable(name: "reset", scope: !7, file: !1, line: 75, type: !10)
!95 = !DILocalVariable(name: "steps", scope: !7, file: !1, line: 76, type: !10)
!96 = !DILocalVariable(name: "strides", scope: !7, file: !1, line: 77, type: !10)
!97 = !DILocalVariable(name: "inStride", scope: !7, file: !1, line: 78, type: !10)
!98 = !DILocalVariable(name: "stepProbability", scope: !7, file: !1, line: 80, type: !99)
!99 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!100 = !DILocalVariable(name: "stepIntersectionProbability", scope: !7, file: !1, line: 81, type: !99)
!101 = !DILocation(line: 81, column: 8, scope: !7)
!102 = !DILocalVariable(name: "fSteps", scope: !7, file: !1, line: 82, type: !99)
!103 = !DILocalVariable(name: "bernoulliSum", scope: !7, file: !1, line: 83, type: !99)
!104 = !DILocalVariable(name: "poissonBinomialAccum", scope: !7, file: !1, line: 84, type: !99)
!105 = !DILocalVariable(name: "timestampPrevious", scope: !7, file: !1, line: 87, type: !99)
!106 = !DILocalVariable(name: "fTimestampPrevious", scope: !7, file: !1, line: 88, type: !99)
!107 = !DILocation(line: 88, column: 8, scope: !7)
!108 = !DILocalVariable(name: "timestampSamples", scope: !7, file: !1, line: 93, type: !109)
!109 = !DICompositeType(tag: DW_TAG_array_type, baseType: !99, size: 128, elements: !110)
!110 = !{!111}
!111 = !DISubrange(count: 4)
!112 = !DILocation(line: 93, column: 8, scope: !7)
!113 = !DILocalVariable(name: "xSamples", scope: !7, file: !1, line: 94, type: !109)
!114 = !DILocation(line: 94, column: 8, scope: !7)
!115 = !DILocalVariable(name: "ySamples", scope: !7, file: !1, line: 95, type: !109)
!116 = !DILocation(line: 95, column: 8, scope: !7)
!117 = !DILocalVariable(name: "zSamples", scope: !7, file: !1, line: 96, type: !109)
!118 = !DILocation(line: 96, column: 8, scope: !7)
!119 = !DILocalVariable(name: "xDCSamples", scope: !7, file: !1, line: 97, type: !120)
!120 = !DICompositeType(tag: DW_TAG_array_type, baseType: !99, size: 1536, elements: !121)
!121 = !{!122}
!122 = !DISubrange(count: 48)
!123 = !DILocation(line: 97, column: 8, scope: !7)
!124 = !DILocalVariable(name: "yDCSamples", scope: !7, file: !1, line: 98, type: !120)
!125 = !DILocation(line: 98, column: 8, scope: !7)
!126 = !DILocalVariable(name: "zDCSamples", scope: !7, file: !1, line: 99, type: !120)
!127 = !DILocation(line: 99, column: 8, scope: !7)
!128 = !DILocalVariable(name: "max_x", scope: !7, file: !1, line: 109, type: !99)
!129 = !DILocalVariable(name: "max_y", scope: !7, file: !1, line: 110, type: !99)
!130 = !DILocalVariable(name: "max_z", scope: !7, file: !1, line: 111, type: !99)
!131 = !DILocalVariable(name: "min_x", scope: !7, file: !1, line: 112, type: !99)
!132 = !DILocalVariable(name: "min_y", scope: !7, file: !1, line: 113, type: !99)
!133 = !DILocalVariable(name: "min_z", scope: !7, file: !1, line: 114, type: !99)
!134 = !DILocalVariable(name: "threshold_x", scope: !7, file: !1, line: 115, type: !99)
!135 = !DILocalVariable(name: "threshold_y", scope: !7, file: !1, line: 116, type: !99)
!136 = !DILocalVariable(name: "threshold_z", scope: !7, file: !1, line: 117, type: !99)
!137 = !DILocalVariable(name: "measurementOld_x", scope: !7, file: !1, line: 119, type: !138)
!138 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055xAcceleration", scope: !7, file: !1, line: 101, baseType: !99)
!139 = !DILocalVariable(name: "measurementOld_y", scope: !7, file: !1, line: 120, type: !140)
!140 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055yAcceleration", scope: !7, file: !1, line: 102, baseType: !99)
!141 = !DILocalVariable(name: "measurementOld_z", scope: !7, file: !1, line: 121, type: !142)
!142 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055zAcceleration", scope: !7, file: !1, line: 103, baseType: !99)
!143 = !DILocalVariable(name: "measurementNew_x", scope: !7, file: !1, line: 122, type: !138)
!144 = !DILocalVariable(name: "measurementNew_y", scope: !7, file: !1, line: 123, type: !140)
!145 = !DILocalVariable(name: "measurementNew_z", scope: !7, file: !1, line: 124, type: !142)
!146 = !DILocalVariable(name: "xDC", scope: !7, file: !1, line: 126, type: !99)
!147 = !DILocalVariable(name: "yDC", scope: !7, file: !1, line: 127, type: !99)
!148 = !DILocalVariable(name: "zDC", scope: !7, file: !1, line: 128, type: !99)
!149 = !DILocalVariable(name: "tv", scope: !7, file: !1, line: 134, type: !150)
!150 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "timeval", file: !151, line: 8, size: 128, elements: !152)
!151 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_timeval.h", directory: "")
!152 = !{!153, !155}
!153 = !DIDerivedType(tag: DW_TAG_member, name: "tv_sec", scope: !150, file: !151, line: 10, baseType: !154, size: 64)
!154 = !DIDerivedType(tag: DW_TAG_typedef, name: "__time_t", file: !52, line: 160, baseType: !53)
!155 = !DIDerivedType(tag: DW_TAG_member, name: "tv_usec", scope: !150, file: !151, line: 11, baseType: !156, size: 64, offset: 64)
!156 = !DIDerivedType(tag: DW_TAG_typedef, name: "__suseconds_t", file: !52, line: 162, baseType: !53)
!157 = !DILocation(line: 134, column: 17, scope: !7)
!158 = !DILocation(line: 135, column: 5, scope: !7)
!159 = !DILocation(line: 136, column: 20, scope: !7)
!160 = !DILocation(line: 136, column: 17, scope: !7)
!161 = !DILocation(line: 136, column: 41, scope: !7)
!162 = !DILocation(line: 136, column: 38, scope: !7)
!163 = !DILocation(line: 136, column: 36, scope: !7)
!164 = !DILocation(line: 136, column: 27, scope: !7)
!165 = !DILocalVariable(name: "t1", scope: !7, file: !1, line: 136, type: !166)
!166 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!167 = !DILocation(line: 138, column: 2, scope: !7)
!168 = !DILocalVariable(name: "timestamp", scope: !7, file: !1, line: 86, type: !99)
!169 = !DILocation(line: 138, column: 9, scope: !7)
!170 = !DILocation(line: 0, scope: !171)
!171 = distinct !DILexicalBlock(scope: !7, file: !1, line: 139, column: 2)
!172 = !DILocation(line: 0, scope: !173)
!173 = distinct !DILexicalBlock(scope: !171, file: !1, line: 153, column: 3)
!174 = !DILocalVariable(name: "i", scope: !173, file: !1, line: 153, type: !77)
!175 = !DILocalVariable(name: "samplesTaken", scope: !171, file: !1, line: 148, type: !10)
!176 = !DILocalVariable(name: "timestampAccum", scope: !171, file: !1, line: 147, type: !99)
!177 = !DILocalVariable(name: "acc_z", scope: !7, file: !1, line: 107, type: !142)
!178 = !DILocalVariable(name: "acc_y", scope: !7, file: !1, line: 106, type: !140)
!179 = !DILocalVariable(name: "acc_x", scope: !7, file: !1, line: 105, type: !138)
!180 = !DILocation(line: 153, column: 24, scope: !181)
!181 = distinct !DILexicalBlock(scope: !173, file: !1, line: 153, column: 3)
!182 = !DILocation(line: 153, column: 3, scope: !173)
!183 = !DILocation(line: 156, column: 60, scope: !184)
!184 = distinct !DILexicalBlock(scope: !181, file: !1, line: 154, column: 3)
!185 = !DILocation(line: 156, column: 82, scope: !184)
!186 = !DILocation(line: 156, column: 96, scope: !184)
!187 = !DILocation(line: 156, column: 110, scope: !184)
!188 = !DILocation(line: 156, column: 20, scope: !184)
!189 = !DILocalVariable(name: "itemsRead", scope: !184, file: !1, line: 156, type: !10)
!190 = !DILocation(line: 0, scope: !184)
!191 = !DILocation(line: 157, column: 25, scope: !192)
!192 = distinct !DILexicalBlock(scope: !184, file: !1, line: 157, column: 8)
!193 = !DILocation(line: 167, column: 13, scope: !194)
!194 = distinct !DILexicalBlock(scope: !195, file: !1, line: 166, column: 4)
!195 = distinct !DILexicalBlock(scope: !192, file: !1, line: 165, column: 13)
!196 = !DILocation(line: 167, column: 5, scope: !194)
!197 = !DILocation(line: 168, column: 4, scope: !194)
!198 = !DILocation(line: 170, column: 19, scope: !184)
!199 = !DILocation(line: 171, column: 16, scope: !184)
!200 = !DILocation(line: 176, column: 13, scope: !184)
!201 = !DILocation(line: 176, column: 10, scope: !184)
!202 = !DILocation(line: 177, column: 13, scope: !184)
!203 = !DILocation(line: 177, column: 10, scope: !184)
!204 = !DILocation(line: 178, column: 13, scope: !184)
!205 = !DILocation(line: 178, column: 10, scope: !184)
!206 = !DILocation(line: 179, column: 16, scope: !184)
!207 = !DILocation(line: 153, column: 52, scope: !181)
!208 = !DILocation(line: 153, column: 3, scope: !181)
!209 = distinct !{!209, !182, !210, !90}
!210 = !DILocation(line: 180, column: 3, scope: !173)
!211 = !DILocalVariable(name: "diff_z", scope: !171, file: !1, line: 195, type: !99)
!212 = !DILocation(line: 195, column: 9, scope: !171)
!213 = !DILocation(line: 201, column: 24, scope: !214)
!214 = distinct !DILexicalBlock(scope: !171, file: !1, line: 201, column: 7)
!215 = !DILocation(line: 201, column: 7, scope: !171)
!216 = !DILocation(line: 229, column: 19, scope: !217)
!217 = distinct !DILexicalBlock(scope: !171, file: !1, line: 229, column: 7)
!218 = !DILocation(line: 229, column: 7, scope: !217)
!219 = !DILocation(line: 229, column: 39, scope: !217)
!220 = !DILocation(line: 229, column: 7, scope: !171)
!221 = distinct !{!221, !167, !222, !90}
!222 = !DILocation(line: 303, column: 2, scope: !7)
!223 = !DILocation(line: 247, column: 24, scope: !224)
!224 = distinct !DILexicalBlock(scope: !171, file: !1, line: 247, column: 7)
!225 = !DILocation(line: 247, column: 7, scope: !171)
!226 = !DILocation(line: 252, column: 13, scope: !227)
!227 = distinct !DILexicalBlock(scope: !171, file: !1, line: 252, column: 7)
!228 = !DILocation(line: 252, column: 7, scope: !171)
!229 = !DILocation(line: 260, column: 18, scope: !171)
!230 = !DILocalVariable(name: "zRange", scope: !7, file: !1, line: 130, type: !99)
!231 = !DILocation(line: 262, column: 24, scope: !232)
!232 = distinct !DILexicalBlock(scope: !171, file: !1, line: 262, column: 7)
!233 = !DILocation(line: 262, column: 29, scope: !232)
!234 = !DILocation(line: 264, column: 25, scope: !235)
!235 = distinct !DILexicalBlock(scope: !232, file: !1, line: 263, column: 3)
!236 = !DILocation(line: 264, column: 34, scope: !235)
!237 = !DILocalVariable(name: "temp_z", scope: !235, file: !1, line: 270, type: !99)
!238 = !DILocation(line: 0, scope: !235)
!239 = !DILocation(line: 275, column: 3, scope: !235)
!240 = !DILocation(line: 280, column: 24, scope: !241)
!241 = distinct !DILexicalBlock(scope: !171, file: !1, line: 280, column: 7)
!242 = !DILocation(line: 281, column: 8, scope: !241)
!243 = !DILocation(line: 282, column: 17, scope: !241)
!244 = !DILocation(line: 282, column: 7, scope: !241)
!245 = !DILocation(line: 282, column: 37, scope: !241)
!246 = !DILocation(line: 283, column: 4, scope: !241)
!247 = !DILocation(line: 285, column: 9, scope: !248)
!248 = distinct !DILexicalBlock(scope: !241, file: !1, line: 284, column: 3)
!249 = !DILocation(line: 289, column: 3, scope: !248)
!250 = !DILocation(line: 291, column: 17, scope: !251)
!251 = distinct !DILexicalBlock(scope: !171, file: !1, line: 291, column: 7)
!252 = !DILocation(line: 291, column: 37, scope: !251)
!253 = !DILocation(line: 291, column: 41, scope: !251)
!254 = !DILocation(line: 302, column: 19, scope: !171)
!255 = !DILocation(line: 305, column: 2, scope: !7)
!256 = !DILocation(line: 306, column: 20, scope: !7)
!257 = !DILocation(line: 306, column: 17, scope: !7)
!258 = !DILocation(line: 306, column: 41, scope: !7)
!259 = !DILocation(line: 306, column: 38, scope: !7)
!260 = !DILocation(line: 306, column: 36, scope: !7)
!261 = !DILocation(line: 306, column: 27, scope: !7)
!262 = !DILocalVariable(name: "t2", scope: !7, file: !1, line: 306, type: !166)
!263 = !DILocation(line: 308, column: 21, scope: !7)
!264 = !DILocation(line: 308, column: 2, scope: !7)
!265 = !DILocation(line: 312, column: 2, scope: !7)
!266 = !DILocation(line: 314, column: 2, scope: !7)
