; ModuleID = 'main.ll'
source_filename = "main.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, %struct._IO_codecvt*, %struct._IO_wide_data*, %struct._IO_FILE*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type opaque
%struct._IO_codecvt = type opaque
%struct._IO_wide_data = type opaque

@.str = private unnamed_addr constant [20 x i8] c"Input filename: %s\0A\00", align 1
@.str.1 = private unnamed_addr constant [2 x i8] c"r\00", align 1
@stderr = external dso_local global %struct._IO_FILE*, align 8
@.str.2 = private unnamed_addr constant [27 x i8] c"Error opening input file.\0A\00", align 1
@.str.3 = private unnamed_addr constant [22 x i8] c"Reading from file...\0A\00", align 1
@.str.4 = private unnamed_addr constant [28 x i8] c"Performing measurements...\0A\00", align 1
@.str.5 = private unnamed_addr constant [17 x i8] c"%G,%G,%G,%G,%*G\0A\00", align 1
@.str.6 = private unnamed_addr constant [62 x i8] c"Unable to read row. Possible problem with input data format.\0A\00", align 1
@.str.7 = private unnamed_addr constant [33 x i8] c"They took %d steps, %d strides.\0A\00", align 1
@.str.8 = private unnamed_addr constant [31 x i8] c"Measurement count total : %d.\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 %0, i8** %1) #0 !dbg !9 {
  %3 = alloca [4 x float], align 16
  %4 = alloca [4 x float], align 16
  %5 = alloca [4 x float], align 16
  %6 = alloca [4 x float], align 16
  call void @llvm.dbg.value(metadata i32 %0, metadata !16, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i8** %1, metadata !18, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata [150 x i8]* undef, metadata !19, metadata !DIExpression()), !dbg !23
  %7 = getelementptr inbounds i8*, i8** %1, i64 1, !dbg !24
  %8 = load i8*, i8** %7, align 8, !dbg !24
  %9 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([20 x i8], [20 x i8]* @.str, i64 0, i64 0), i8* %8), !dbg !25
  %10 = getelementptr inbounds i8*, i8** %1, i64 1, !dbg !26
  %11 = load i8*, i8** %10, align 8, !dbg !26
  %12 = call %struct._IO_FILE* @fopen(i8* %11, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.1, i64 0, i64 0)), !dbg !27
  call void @llvm.dbg.value(metadata %struct._IO_FILE* %12, metadata !28, metadata !DIExpression()), !dbg !17
  %13 = icmp eq %struct._IO_FILE* %12, null, !dbg !88
  br i1 %13, label %14, label %17, !dbg !90

14:                                               ; preds = %2
  %15 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 8, !dbg !91
  %16 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %15, i8* getelementptr inbounds ([27 x i8], [27 x i8]* @.str.2, i64 0, i64 0)), !dbg !93
  call void @exit(i32 1) #4, !dbg !94
  unreachable, !dbg !94

17:                                               ; preds = %2
  br label %18, !dbg !95

18:                                               ; preds = %21, %17
  %19 = call i32 @fgetc(%struct._IO_FILE* %12), !dbg !96
  %20 = icmp ne i32 %19, 10, !dbg !97
  br i1 %20, label %21, label %22, !dbg !95

21:                                               ; preds = %18
  br label %18, !dbg !95, !llvm.loop !98

22:                                               ; preds = %18
  %23 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([22 x i8], [22 x i8]* @.str.3, i64 0, i64 0)), !dbg !101
  call void @llvm.dbg.value(metadata i32 0, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 -1, metadata !103, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !104, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 1, metadata !105, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !106, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !107, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 1, metadata !108, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !109, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata float* undef, metadata !111, metadata !DIExpression()), !dbg !112
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !113, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !114, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !115, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float -1.000000e+03, metadata !116, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata float* undef, metadata !117, metadata !DIExpression()), !dbg !118
  call void @llvm.dbg.declare(metadata [4 x float]* %3, metadata !119, metadata !DIExpression()), !dbg !123
  call void @llvm.dbg.declare(metadata [4 x float]* %4, metadata !124, metadata !DIExpression()), !dbg !125
  call void @llvm.dbg.declare(metadata [4 x float]* %5, metadata !126, metadata !DIExpression()), !dbg !127
  call void @llvm.dbg.declare(metadata [4 x float]* %6, metadata !128, metadata !DIExpression()), !dbg !129
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !130, metadata !DIExpression()), !dbg !134
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !135, metadata !DIExpression()), !dbg !136
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !137, metadata !DIExpression()), !dbg !138
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !139, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !140, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !141, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !142, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !143, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !144, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !145, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !146, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !147, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !148, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !150, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !152, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !154, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !155, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !156, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !157, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !158, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !159, metadata !DIExpression()), !dbg !17
  %24 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([28 x i8], [28 x i8]* @.str.4, i64 0, i64 0)), !dbg !160
  br label %25, !dbg !161

25:                                               ; preds = %109, %72, %22
  %.026 = phi float [ -1.000000e+03, %22 ], [ %.127, %109 ], [ %.026, %72 ], !dbg !17
  %.024 = phi float [ undef, %22 ], [ %.125, %109 ], [ %.125, %72 ]
  %.019 = phi float [ -1.000000e+04, %22 ], [ %.221, %109 ], [ %.019, %72 ], !dbg !17
  %.016 = phi float [ 1.000000e+04, %22 ], [ %.218, %109 ], [ %.016, %72 ], !dbg !17
  %.014 = phi float [ 0.000000e+00, %22 ], [ %.115, %109 ], [ %.014, %72 ], !dbg !17
  %.012 = phi i32 [ 0, %22 ], [ %.113, %109 ], [ %.012, %72 ], !dbg !17
  %.010 = phi i32 [ 0, %22 ], [ %.111, %109 ], [ %.010, %72 ], !dbg !17
  %.08 = phi i32 [ 1, %22 ], [ %.2, %109 ], [ %.08, %72 ], !dbg !17
  %.06 = phi i32 [ -1, %22 ], [ %110, %109 ], [ %.17, %72 ], !dbg !17
  %.04 = phi i32 [ 0, %22 ], [ %.15, %109 ], [ %.15, %72 ], !dbg !17
  %.03 = phi float [ 0.000000e+00, %22 ], [ %.023, %109 ], [ %.1, %72 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.03, metadata !156, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.04, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.06, metadata !103, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.08, metadata !105, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.010, metadata !106, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.012, metadata !107, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.014, metadata !147, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.016, metadata !144, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.019, metadata !141, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.024, metadata !162, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.026, metadata !116, metadata !DIExpression()), !dbg !17
  %26 = icmp ne i32 %.04, 0, !dbg !163
  %27 = xor i1 %26, true, !dbg !163
  br i1 %27, label %28, label %111, !dbg !161

28:                                               ; preds = %25
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !164, metadata !DIExpression()), !dbg !166
  call void @llvm.dbg.value(metadata i32 0, metadata !167, metadata !DIExpression()), !dbg !166
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !168, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !169, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !170, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i64 0, metadata !171, metadata !DIExpression()), !dbg !173
  br label %29, !dbg !174

29:                                               ; preds = %61, %28
  %.028 = phi float [ 0.000000e+00, %28 ], [ %52, %61 ], !dbg !166
  %.125 = phi float [ %.024, %28 ], [ %60, %61 ]
  %.023 = phi float [ 0.000000e+00, %28 ], [ %55, %61 ], !dbg !166
  %.022 = phi float [ 0.000000e+00, %28 ], [ %58, %61 ], !dbg !166
  %.02 = phi float [ 0.000000e+00, %28 ], [ %48, %61 ], !dbg !166
  %.01 = phi i32 [ 0, %28 ], [ %49, %61 ], !dbg !166
  %.0 = phi i64 [ 0, %28 ], [ %62, %61 ], !dbg !173
  call void @llvm.dbg.value(metadata i64 %.0, metadata !171, metadata !DIExpression()), !dbg !173
  call void @llvm.dbg.value(metadata i32 %.01, metadata !167, metadata !DIExpression()), !dbg !166
  call void @llvm.dbg.value(metadata float %.02, metadata !164, metadata !DIExpression()), !dbg !166
  call void @llvm.dbg.value(metadata float %.022, metadata !170, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.023, metadata !169, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.125, metadata !162, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.028, metadata !168, metadata !DIExpression()), !dbg !17
  %30 = icmp ult i64 %.0, 4, !dbg !175
  br i1 %30, label %31, label %63, !dbg !177

31:                                               ; preds = %29
  %32 = getelementptr inbounds [4 x float], [4 x float]* %3, i64 0, i64 %.0, !dbg !178
  %33 = getelementptr inbounds [4 x float], [4 x float]* %4, i64 0, i64 %.0, !dbg !180
  %34 = getelementptr inbounds [4 x float], [4 x float]* %5, i64 0, i64 %.0, !dbg !181
  %35 = getelementptr inbounds [4 x float], [4 x float]* %6, i64 0, i64 %.0, !dbg !182
  %36 = call i32 (%struct._IO_FILE*, i8*, ...) @__isoc99_fscanf(%struct._IO_FILE* %12, i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.5, i64 0, i64 0), float* %32, float* %33, float* %34, float* %35), !dbg !183
  call void @llvm.dbg.value(metadata i32 %36, metadata !184, metadata !DIExpression()), !dbg !185
  %37 = icmp eq i32 %36, -1, !dbg !186
  br i1 %37, label %40, label %38, !dbg !188

38:                                               ; preds = %31
  %39 = icmp eq i32 %36, 0, !dbg !189
  br i1 %39, label %40, label %41, !dbg !190

40:                                               ; preds = %38, %31
  call void @llvm.dbg.value(metadata i32 1, metadata !102, metadata !DIExpression()), !dbg !17
  br label %63, !dbg !191

41:                                               ; preds = %38
  %42 = icmp ne i32 %36, 4, !dbg !193
  br i1 %42, label %43, label %46, !dbg !195

43:                                               ; preds = %41
  %44 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 8, !dbg !196
  %45 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %44, i8* getelementptr inbounds ([62 x i8], [62 x i8]* @.str.6, i64 0, i64 0)), !dbg !198
  br label %46, !dbg !199

46:                                               ; preds = %43, %41
  br label %47

47:                                               ; preds = %46
  %48 = fadd float %.02, %.125, !dbg !200
  call void @llvm.dbg.value(metadata float %48, metadata !164, metadata !DIExpression()), !dbg !166
  %49 = add nsw i32 %.01, 1, !dbg !201
  call void @llvm.dbg.value(metadata i32 %49, metadata !167, metadata !DIExpression()), !dbg !166
  %50 = getelementptr inbounds [4 x float], [4 x float]* %4, i64 0, i64 %.0, !dbg !202
  %51 = load float, float* %50, align 4, !dbg !202
  %52 = fadd float %.028, %51, !dbg !203
  call void @llvm.dbg.value(metadata float %52, metadata !168, metadata !DIExpression()), !dbg !17
  %53 = getelementptr inbounds [4 x float], [4 x float]* %5, i64 0, i64 %.0, !dbg !204
  %54 = load float, float* %53, align 4, !dbg !204
  %55 = fadd float %.023, %54, !dbg !205
  call void @llvm.dbg.value(metadata float %55, metadata !169, metadata !DIExpression()), !dbg !17
  %56 = getelementptr inbounds [4 x float], [4 x float]* %6, i64 0, i64 %.0, !dbg !206
  %57 = load float, float* %56, align 4, !dbg !206
  %58 = fadd float %.022, %57, !dbg !207
  call void @llvm.dbg.value(metadata float %58, metadata !170, metadata !DIExpression()), !dbg !17
  %59 = getelementptr inbounds [4 x float], [4 x float]* %3, i64 0, i64 %.0, !dbg !208
  %60 = load float, float* %59, align 4, !dbg !208
  call void @llvm.dbg.value(metadata float %60, metadata !162, metadata !DIExpression()), !dbg !17
  br label %61, !dbg !209

61:                                               ; preds = %47
  %62 = add i64 %.0, 1, !dbg !210
  call void @llvm.dbg.value(metadata i64 %62, metadata !171, metadata !DIExpression()), !dbg !173
  br label %29, !dbg !211, !llvm.loop !212

63:                                               ; preds = %40, %29
  %.15 = phi i32 [ 1, %40 ], [ %.04, %29 ], !dbg !17
  call void @llvm.dbg.value(metadata i32 %.15, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata float* undef, metadata !214, metadata !DIExpression()), !dbg !215
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !148, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !150, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.03, metadata !152, metadata !DIExpression()), !dbg !17
  %64 = icmp eq i32 %.06, -1, !dbg !216
  br i1 %64, label %65, label %66, !dbg !218

65:                                               ; preds = %63
  call void @llvm.dbg.value(metadata float %.023, metadata !156, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !103, metadata !DIExpression()), !dbg !17
  br label %66, !dbg !219

66:                                               ; preds = %65, %63
  %.17 = phi i32 [ 0, %65 ], [ %.06, %63 ], !dbg !17
  %.1 = phi float [ %.023, %65 ], [ %.03, %63 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.1, metadata !156, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.17, metadata !103, metadata !DIExpression()), !dbg !17
  %67 = fsub float %.023, %.1, !dbg !221
  %68 = call float @llvm.fabs.f32(float %67), !dbg !223
  %69 = fpext float %68 to double, !dbg !223
  %70 = fcmp ogt double %69, 5.000000e-01, !dbg !224
  br i1 %70, label %71, label %72, !dbg !225

71:                                               ; preds = %66
  call void @llvm.dbg.value(metadata float %.023, metadata !156, metadata !DIExpression()), !dbg !17
  br label %73, !dbg !226

72:                                               ; preds = %66
  br label %25, !dbg !228, !llvm.loop !230

73:                                               ; preds = %71
  %74 = fcmp ogt float %.023, %.019, !dbg !232
  br i1 %74, label %75, label %76, !dbg !234

75:                                               ; preds = %73
  call void @llvm.dbg.value(metadata float %.023, metadata !141, metadata !DIExpression()), !dbg !17
  br label %76, !dbg !235

76:                                               ; preds = %75, %73
  %.120 = phi float [ %.023, %75 ], [ %.019, %73 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.120, metadata !141, metadata !DIExpression()), !dbg !17
  %77 = fcmp ogt float %.016, %.023, !dbg !237
  br i1 %77, label %78, label %79, !dbg !239

78:                                               ; preds = %76
  call void @llvm.dbg.value(metadata float %.023, metadata !144, metadata !DIExpression()), !dbg !17
  br label %79, !dbg !240

79:                                               ; preds = %78, %76
  %.117 = phi float [ %.023, %78 ], [ %.016, %76 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.117, metadata !144, metadata !DIExpression()), !dbg !17
  %80 = fsub float %.120, %.117, !dbg !242
  call void @llvm.dbg.value(metadata float %80, metadata !243, metadata !DIExpression()), !dbg !17
  %81 = icmp slt i32 %.17, 10, !dbg !244
  br i1 %81, label %85, label %82, !dbg !246

82:                                               ; preds = %79
  %83 = srem i32 %.17, 50, !dbg !247
  %84 = icmp ne i32 %83, 0, !dbg !247
  br i1 %84, label %85, label %88, !dbg !248

85:                                               ; preds = %82, %79
  %86 = fadd float %.120, %.117, !dbg !249
  %87 = fdiv float %86, 2.000000e+00, !dbg !251
  call void @llvm.dbg.value(metadata float %87, metadata !147, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.120, metadata !252, metadata !DIExpression()), !dbg !253
  call void @llvm.dbg.value(metadata float %.117, metadata !141, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.120, metadata !144, metadata !DIExpression()), !dbg !17
  br label %88, !dbg !254

88:                                               ; preds = %85, %82
  %.221 = phi float [ %.117, %85 ], [ %.120, %82 ], !dbg !166
  %.218 = phi float [ %.120, %85 ], [ %.117, %82 ], !dbg !166
  %.115 = phi float [ %87, %85 ], [ %.014, %82 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.115, metadata !147, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.218, metadata !144, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.221, metadata !141, metadata !DIExpression()), !dbg !17
  %89 = fcmp ogt float %.03, %.023, !dbg !255
  br i1 %89, label %90, label %102, !dbg !257

90:                                               ; preds = %88
  %91 = fcmp ogt float %.115, %.023, !dbg !258
  br i1 %91, label %92, label %102, !dbg !259

92:                                               ; preds = %90
  %93 = fsub float %.125, %.026, !dbg !260
  %94 = fpext float %93 to double, !dbg !261
  %95 = fcmp ogt double %94, 3.000000e-01, !dbg !262
  br i1 %95, label %96, label %102, !dbg !263

96:                                               ; preds = %92
  %97 = fcmp ogt float %80, 2.000000e+00, !dbg !264
  br i1 %97, label %98, label %102, !dbg !265

98:                                               ; preds = %96
  %99 = fcmp olt float %80, 8.000000e+00, !dbg !266
  br i1 %99, label %100, label %102, !dbg !267

100:                                              ; preds = %98
  %101 = add nsw i32 %.010, 1, !dbg !268
  call void @llvm.dbg.value(metadata i32 %101, metadata !106, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !105, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.125, metadata !116, metadata !DIExpression()), !dbg !17
  br label %102, !dbg !270

102:                                              ; preds = %100, %98, %96, %92, %90, %88
  %.127 = phi float [ %.125, %100 ], [ %.026, %98 ], [ %.026, %96 ], [ %.026, %92 ], [ %.026, %90 ], [ %.026, %88 ], !dbg !17
  %.111 = phi i32 [ %101, %100 ], [ %.010, %98 ], [ %.010, %96 ], [ %.010, %92 ], [ %.010, %90 ], [ %.010, %88 ], !dbg !17
  %.19 = phi i32 [ 0, %100 ], [ %.08, %98 ], [ %.08, %96 ], [ %.08, %92 ], [ %.08, %90 ], [ %.08, %88 ], !dbg !17
  call void @llvm.dbg.value(metadata i32 %.19, metadata !105, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.111, metadata !106, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.127, metadata !116, metadata !DIExpression()), !dbg !17
  %103 = fsub float %.125, %.127, !dbg !271
  %104 = fcmp ogt float %103, 2.000000e+00, !dbg !273
  br i1 %104, label %105, label %109, !dbg !274

105:                                              ; preds = %102
  %106 = icmp eq i32 %.19, 0, !dbg !275
  br i1 %106, label %107, label %109, !dbg !276

107:                                              ; preds = %105
  %108 = add nsw i32 %.012, 1, !dbg !277
  call void @llvm.dbg.value(metadata i32 %108, metadata !107, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 1, metadata !105, metadata !DIExpression()), !dbg !17
  br label %109, !dbg !279

109:                                              ; preds = %107, %105, %102
  %.113 = phi i32 [ %108, %107 ], [ %.012, %105 ], [ %.012, %102 ], !dbg !17
  %.2 = phi i32 [ 1, %107 ], [ %.19, %105 ], [ %.19, %102 ], !dbg !166
  call void @llvm.dbg.value(metadata i32 %.2, metadata !105, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.113, metadata !107, metadata !DIExpression()), !dbg !17
  %110 = add nsw i32 %.17, 1, !dbg !280
  call void @llvm.dbg.value(metadata i32 %110, metadata !103, metadata !DIExpression()), !dbg !17
  br label %25, !dbg !161, !llvm.loop !230

111:                                              ; preds = %25
  %112 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([33 x i8], [33 x i8]* @.str.7, i64 0, i64 0), i32 %.010, i32 %.012), !dbg !281
  %113 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([31 x i8], [31 x i8]* @.str.8, i64 0, i64 0), i32 %.06), !dbg !282
  %114 = call i32 @fclose(%struct._IO_FILE* %12), !dbg !283
  ret i32 0, !dbg !284
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare dso_local i32 @printf(i8*, ...) #2

declare dso_local %struct._IO_FILE* @fopen(i8*, i8*) #2

declare dso_local i32 @fprintf(%struct._IO_FILE*, i8*, ...) #2

; Function Attrs: noreturn nounwind
declare dso_local void @exit(i32) #3

declare dso_local i32 @fgetc(%struct._IO_FILE*) #2

declare dso_local i32 @__isoc99_fscanf(%struct._IO_FILE*, i8*, ...) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.fabs.f32(float) #1

declare dso_local i32 @fclose(%struct._IO_FILE*) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noreturn nounwind "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { noreturn nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!5, !6, !7}
!llvm.ident = !{!8}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !3, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "main.c", directory: "/home/blackgeorge/Noisy-lang-compiler/applications/newton/llvm-ir/pedometer")
!2 = !{}
!3 = !{!4}
!4 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!5 = !{i32 7, !"Dwarf Version", i32 4}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = !{i32 1, !"wchar_size", i32 4}
!8 = !{!"Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11"}
!9 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 49, type: !10, scopeLine: 50, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!10 = !DISubroutineType(types: !11)
!11 = !{!12, !12, !13}
!12 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!13 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !14, size: 64)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!16 = !DILocalVariable(name: "argc", arg: 1, scope: !9, file: !1, line: 49, type: !12)
!17 = !DILocation(line: 0, scope: !9)
!18 = !DILocalVariable(name: "argv", arg: 2, scope: !9, file: !1, line: 49, type: !13)
!19 = !DILocalVariable(name: "charBuffer", scope: !9, file: !1, line: 51, type: !20)
!20 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 1200, elements: !21)
!21 = !{!22}
!22 = !DISubrange(count: 150)
!23 = !DILocation(line: 51, column: 7, scope: !9)
!24 = !DILocation(line: 54, column: 33, scope: !9)
!25 = !DILocation(line: 54, column: 2, scope: !9)
!26 = !DILocation(line: 56, column: 20, scope: !9)
!27 = !DILocation(line: 56, column: 14, scope: !9)
!28 = !DILocalVariable(name: "inputFile", scope: !9, file: !1, line: 52, type: !29)
!29 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !30, size: 64)
!30 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !31, line: 7, baseType: !32)
!31 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/FILE.h", directory: "")
!32 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_FILE", file: !33, line: 49, size: 1728, elements: !34)
!33 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h", directory: "")
!34 = !{!35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !50, !52, !53, !54, !58, !60, !62, !66, !69, !71, !74, !77, !78, !79, !83, !84}
!35 = !DIDerivedType(tag: DW_TAG_member, name: "_flags", scope: !32, file: !33, line: 51, baseType: !12, size: 32)
!36 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_ptr", scope: !32, file: !33, line: 54, baseType: !14, size: 64, offset: 64)
!37 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_end", scope: !32, file: !33, line: 55, baseType: !14, size: 64, offset: 128)
!38 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_base", scope: !32, file: !33, line: 56, baseType: !14, size: 64, offset: 192)
!39 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_base", scope: !32, file: !33, line: 57, baseType: !14, size: 64, offset: 256)
!40 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_ptr", scope: !32, file: !33, line: 58, baseType: !14, size: 64, offset: 320)
!41 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_end", scope: !32, file: !33, line: 59, baseType: !14, size: 64, offset: 384)
!42 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_base", scope: !32, file: !33, line: 60, baseType: !14, size: 64, offset: 448)
!43 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_end", scope: !32, file: !33, line: 61, baseType: !14, size: 64, offset: 512)
!44 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_base", scope: !32, file: !33, line: 64, baseType: !14, size: 64, offset: 576)
!45 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_backup_base", scope: !32, file: !33, line: 65, baseType: !14, size: 64, offset: 640)
!46 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_end", scope: !32, file: !33, line: 66, baseType: !14, size: 64, offset: 704)
!47 = !DIDerivedType(tag: DW_TAG_member, name: "_markers", scope: !32, file: !33, line: 68, baseType: !48, size: 64, offset: 768)
!48 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !49, size: 64)
!49 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_marker", file: !33, line: 36, flags: DIFlagFwdDecl)
!50 = !DIDerivedType(tag: DW_TAG_member, name: "_chain", scope: !32, file: !33, line: 70, baseType: !51, size: 64, offset: 832)
!51 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !32, size: 64)
!52 = !DIDerivedType(tag: DW_TAG_member, name: "_fileno", scope: !32, file: !33, line: 72, baseType: !12, size: 32, offset: 896)
!53 = !DIDerivedType(tag: DW_TAG_member, name: "_flags2", scope: !32, file: !33, line: 73, baseType: !12, size: 32, offset: 928)
!54 = !DIDerivedType(tag: DW_TAG_member, name: "_old_offset", scope: !32, file: !33, line: 74, baseType: !55, size: 64, offset: 960)
!55 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off_t", file: !56, line: 152, baseType: !57)
!56 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types.h", directory: "")
!57 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!58 = !DIDerivedType(tag: DW_TAG_member, name: "_cur_column", scope: !32, file: !33, line: 77, baseType: !59, size: 16, offset: 1024)
!59 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!60 = !DIDerivedType(tag: DW_TAG_member, name: "_vtable_offset", scope: !32, file: !33, line: 78, baseType: !61, size: 8, offset: 1040)
!61 = !DIBasicType(name: "signed char", size: 8, encoding: DW_ATE_signed_char)
!62 = !DIDerivedType(tag: DW_TAG_member, name: "_shortbuf", scope: !32, file: !33, line: 79, baseType: !63, size: 8, offset: 1048)
!63 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 8, elements: !64)
!64 = !{!65}
!65 = !DISubrange(count: 1)
!66 = !DIDerivedType(tag: DW_TAG_member, name: "_lock", scope: !32, file: !33, line: 81, baseType: !67, size: 64, offset: 1088)
!67 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !68, size: 64)
!68 = !DIDerivedType(tag: DW_TAG_typedef, name: "_IO_lock_t", file: !33, line: 43, baseType: null)
!69 = !DIDerivedType(tag: DW_TAG_member, name: "_offset", scope: !32, file: !33, line: 89, baseType: !70, size: 64, offset: 1152)
!70 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off64_t", file: !56, line: 153, baseType: !57)
!71 = !DIDerivedType(tag: DW_TAG_member, name: "_codecvt", scope: !32, file: !33, line: 91, baseType: !72, size: 64, offset: 1216)
!72 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !73, size: 64)
!73 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_codecvt", file: !33, line: 37, flags: DIFlagFwdDecl)
!74 = !DIDerivedType(tag: DW_TAG_member, name: "_wide_data", scope: !32, file: !33, line: 92, baseType: !75, size: 64, offset: 1280)
!75 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !76, size: 64)
!76 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_wide_data", file: !33, line: 38, flags: DIFlagFwdDecl)
!77 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_list", scope: !32, file: !33, line: 93, baseType: !51, size: 64, offset: 1344)
!78 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_buf", scope: !32, file: !33, line: 94, baseType: !4, size: 64, offset: 1408)
!79 = !DIDerivedType(tag: DW_TAG_member, name: "__pad5", scope: !32, file: !33, line: 95, baseType: !80, size: 64, offset: 1472)
!80 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !81, line: 46, baseType: !82)
!81 = !DIFile(filename: "/usr/lib/llvm-12/lib/clang/12.0.1/include/stddef.h", directory: "")
!82 = !DIBasicType(name: "long unsigned int", size: 64, encoding: DW_ATE_unsigned)
!83 = !DIDerivedType(tag: DW_TAG_member, name: "_mode", scope: !32, file: !33, line: 96, baseType: !12, size: 32, offset: 1536)
!84 = !DIDerivedType(tag: DW_TAG_member, name: "_unused2", scope: !32, file: !33, line: 98, baseType: !85, size: 160, offset: 1568)
!85 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 160, elements: !86)
!86 = !{!87}
!87 = !DISubrange(count: 20)
!88 = !DILocation(line: 57, column: 16, scope: !89)
!89 = distinct !DILexicalBlock(scope: !9, file: !1, line: 57, column: 6)
!90 = !DILocation(line: 57, column: 6, scope: !9)
!91 = !DILocation(line: 59, column: 11, scope: !92)
!92 = distinct !DILexicalBlock(scope: !89, file: !1, line: 58, column: 2)
!93 = !DILocation(line: 59, column: 3, scope: !92)
!94 = !DILocation(line: 60, column: 3, scope: !92)
!95 = !DILocation(line: 66, column: 2, scope: !9)
!96 = !DILocation(line: 66, column: 9, scope: !9)
!97 = !DILocation(line: 66, column: 26, scope: !9)
!98 = distinct !{!98, !95, !99, !100}
!99 = !DILocation(line: 66, column: 34, scope: !9)
!100 = !{!"llvm.loop.mustprogress"}
!101 = !DILocation(line: 67, column: 2, scope: !9)
!102 = !DILocalVariable(name: "reachedEOF", scope: !9, file: !1, line: 69, type: !12)
!103 = !DILocalVariable(name: "measurementCount", scope: !9, file: !1, line: 71, type: !12)
!104 = !DILocalVariable(name: "DCEstimateCounter", scope: !9, file: !1, line: 72, type: !12)
!105 = !DILocalVariable(name: "reset", scope: !9, file: !1, line: 74, type: !12)
!106 = !DILocalVariable(name: "steps", scope: !9, file: !1, line: 75, type: !12)
!107 = !DILocalVariable(name: "strides", scope: !9, file: !1, line: 76, type: !12)
!108 = !DILocalVariable(name: "inStride", scope: !9, file: !1, line: 77, type: !12)
!109 = !DILocalVariable(name: "stepProbability", scope: !9, file: !1, line: 79, type: !110)
!110 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!111 = !DILocalVariable(name: "stepIntersectionProbability", scope: !9, file: !1, line: 80, type: !110)
!112 = !DILocation(line: 80, column: 8, scope: !9)
!113 = !DILocalVariable(name: "fSteps", scope: !9, file: !1, line: 81, type: !110)
!114 = !DILocalVariable(name: "bernoulliSum", scope: !9, file: !1, line: 82, type: !110)
!115 = !DILocalVariable(name: "poissonBinomialAccum", scope: !9, file: !1, line: 83, type: !110)
!116 = !DILocalVariable(name: "timestampPrevious", scope: !9, file: !1, line: 86, type: !110)
!117 = !DILocalVariable(name: "fTimestampPrevious", scope: !9, file: !1, line: 87, type: !110)
!118 = !DILocation(line: 87, column: 8, scope: !9)
!119 = !DILocalVariable(name: "timestampSamples", scope: !9, file: !1, line: 92, type: !120)
!120 = !DICompositeType(tag: DW_TAG_array_type, baseType: !110, size: 128, elements: !121)
!121 = !{!122}
!122 = !DISubrange(count: 4)
!123 = !DILocation(line: 92, column: 8, scope: !9)
!124 = !DILocalVariable(name: "xSamples", scope: !9, file: !1, line: 93, type: !120)
!125 = !DILocation(line: 93, column: 8, scope: !9)
!126 = !DILocalVariable(name: "ySamples", scope: !9, file: !1, line: 94, type: !120)
!127 = !DILocation(line: 94, column: 8, scope: !9)
!128 = !DILocalVariable(name: "zSamples", scope: !9, file: !1, line: 95, type: !120)
!129 = !DILocation(line: 95, column: 8, scope: !9)
!130 = !DILocalVariable(name: "xDCSamples", scope: !9, file: !1, line: 96, type: !131)
!131 = !DICompositeType(tag: DW_TAG_array_type, baseType: !110, size: 1536, elements: !132)
!132 = !{!133}
!133 = !DISubrange(count: 48)
!134 = !DILocation(line: 96, column: 8, scope: !9)
!135 = !DILocalVariable(name: "yDCSamples", scope: !9, file: !1, line: 97, type: !131)
!136 = !DILocation(line: 97, column: 8, scope: !9)
!137 = !DILocalVariable(name: "zDCSamples", scope: !9, file: !1, line: 98, type: !131)
!138 = !DILocation(line: 98, column: 8, scope: !9)
!139 = !DILocalVariable(name: "max_x", scope: !9, file: !1, line: 108, type: !110)
!140 = !DILocalVariable(name: "max_y", scope: !9, file: !1, line: 109, type: !110)
!141 = !DILocalVariable(name: "max_z", scope: !9, file: !1, line: 110, type: !110)
!142 = !DILocalVariable(name: "min_x", scope: !9, file: !1, line: 111, type: !110)
!143 = !DILocalVariable(name: "min_y", scope: !9, file: !1, line: 112, type: !110)
!144 = !DILocalVariable(name: "min_z", scope: !9, file: !1, line: 113, type: !110)
!145 = !DILocalVariable(name: "threshold_x", scope: !9, file: !1, line: 114, type: !110)
!146 = !DILocalVariable(name: "threshold_y", scope: !9, file: !1, line: 115, type: !110)
!147 = !DILocalVariable(name: "threshold_z", scope: !9, file: !1, line: 116, type: !110)
!148 = !DILocalVariable(name: "measurementOld_x", scope: !9, file: !1, line: 118, type: !149)
!149 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055xAcceleration", scope: !9, file: !1, line: 100, baseType: !110)
!150 = !DILocalVariable(name: "measurementOld_y", scope: !9, file: !1, line: 119, type: !151)
!151 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055yAcceleration", scope: !9, file: !1, line: 101, baseType: !110)
!152 = !DILocalVariable(name: "measurementOld_z", scope: !9, file: !1, line: 120, type: !153)
!153 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055zAcceleration", scope: !9, file: !1, line: 102, baseType: !110)
!154 = !DILocalVariable(name: "measurementNew_x", scope: !9, file: !1, line: 121, type: !149)
!155 = !DILocalVariable(name: "measurementNew_y", scope: !9, file: !1, line: 122, type: !151)
!156 = !DILocalVariable(name: "measurementNew_z", scope: !9, file: !1, line: 123, type: !153)
!157 = !DILocalVariable(name: "xDC", scope: !9, file: !1, line: 125, type: !110)
!158 = !DILocalVariable(name: "yDC", scope: !9, file: !1, line: 126, type: !110)
!159 = !DILocalVariable(name: "zDC", scope: !9, file: !1, line: 127, type: !110)
!160 = !DILocation(line: 131, column: 2, scope: !9)
!161 = !DILocation(line: 132, column: 2, scope: !9)
!162 = !DILocalVariable(name: "timestamp", scope: !9, file: !1, line: 85, type: !110)
!163 = !DILocation(line: 132, column: 9, scope: !9)
!164 = !DILocalVariable(name: "timestampAccum", scope: !165, file: !1, line: 141, type: !110)
!165 = distinct !DILexicalBlock(scope: !9, file: !1, line: 133, column: 2)
!166 = !DILocation(line: 0, scope: !165)
!167 = !DILocalVariable(name: "samplesTaken", scope: !165, file: !1, line: 142, type: !12)
!168 = !DILocalVariable(name: "acc_x", scope: !9, file: !1, line: 104, type: !149)
!169 = !DILocalVariable(name: "acc_y", scope: !9, file: !1, line: 105, type: !151)
!170 = !DILocalVariable(name: "acc_z", scope: !9, file: !1, line: 106, type: !153)
!171 = !DILocalVariable(name: "i", scope: !172, file: !1, line: 147, type: !80)
!172 = distinct !DILexicalBlock(scope: !165, file: !1, line: 147, column: 3)
!173 = !DILocation(line: 0, scope: !172)
!174 = !DILocation(line: 147, column: 8, scope: !172)
!175 = !DILocation(line: 147, column: 24, scope: !176)
!176 = distinct !DILexicalBlock(scope: !172, file: !1, line: 147, column: 3)
!177 = !DILocation(line: 147, column: 3, scope: !172)
!178 = !DILocation(line: 150, column: 60, scope: !179)
!179 = distinct !DILexicalBlock(scope: !176, file: !1, line: 148, column: 3)
!180 = !DILocation(line: 150, column: 82, scope: !179)
!181 = !DILocation(line: 150, column: 96, scope: !179)
!182 = !DILocation(line: 150, column: 110, scope: !179)
!183 = !DILocation(line: 150, column: 20, scope: !179)
!184 = !DILocalVariable(name: "itemsRead", scope: !179, file: !1, line: 150, type: !12)
!185 = !DILocation(line: 0, scope: !179)
!186 = !DILocation(line: 151, column: 18, scope: !187)
!187 = distinct !DILexicalBlock(scope: !179, file: !1, line: 151, column: 8)
!188 = !DILocation(line: 151, column: 25, scope: !187)
!189 = !DILocation(line: 151, column: 38, scope: !187)
!190 = !DILocation(line: 151, column: 8, scope: !179)
!191 = !DILocation(line: 157, column: 5, scope: !192)
!192 = distinct !DILexicalBlock(scope: !187, file: !1, line: 152, column: 4)
!193 = !DILocation(line: 159, column: 23, scope: !194)
!194 = distinct !DILexicalBlock(scope: !187, file: !1, line: 159, column: 13)
!195 = !DILocation(line: 159, column: 13, scope: !187)
!196 = !DILocation(line: 161, column: 13, scope: !197)
!197 = distinct !DILexicalBlock(scope: !194, file: !1, line: 160, column: 4)
!198 = !DILocation(line: 161, column: 5, scope: !197)
!199 = !DILocation(line: 162, column: 4, scope: !197)
!200 = !DILocation(line: 164, column: 19, scope: !179)
!201 = !DILocation(line: 165, column: 16, scope: !179)
!202 = !DILocation(line: 170, column: 13, scope: !179)
!203 = !DILocation(line: 170, column: 10, scope: !179)
!204 = !DILocation(line: 171, column: 13, scope: !179)
!205 = !DILocation(line: 171, column: 10, scope: !179)
!206 = !DILocation(line: 172, column: 13, scope: !179)
!207 = !DILocation(line: 172, column: 10, scope: !179)
!208 = !DILocation(line: 173, column: 16, scope: !179)
!209 = !DILocation(line: 174, column: 3, scope: !179)
!210 = !DILocation(line: 147, column: 52, scope: !176)
!211 = !DILocation(line: 147, column: 3, scope: !176)
!212 = distinct !{!212, !177, !213, !100}
!213 = !DILocation(line: 174, column: 3, scope: !172)
!214 = !DILocalVariable(name: "diff_z", scope: !165, file: !1, line: 189, type: !110)
!215 = !DILocation(line: 189, column: 9, scope: !165)
!216 = !DILocation(line: 195, column: 24, scope: !217)
!217 = distinct !DILexicalBlock(scope: !165, file: !1, line: 195, column: 7)
!218 = !DILocation(line: 195, column: 7, scope: !165)
!219 = !DILocation(line: 199, column: 3, scope: !220)
!220 = distinct !DILexicalBlock(scope: !217, file: !1, line: 196, column: 3)
!221 = !DILocation(line: 223, column: 19, scope: !222)
!222 = distinct !DILexicalBlock(scope: !165, file: !1, line: 223, column: 7)
!223 = !DILocation(line: 223, column: 7, scope: !222)
!224 = !DILocation(line: 223, column: 39, scope: !222)
!225 = !DILocation(line: 223, column: 7, scope: !165)
!226 = !DILocation(line: 227, column: 3, scope: !227)
!227 = distinct !DILexicalBlock(scope: !222, file: !1, line: 224, column: 3)
!228 = !DILocation(line: 235, column: 4, scope: !229)
!229 = distinct !DILexicalBlock(scope: !222, file: !1, line: 229, column: 3)
!230 = distinct !{!230, !161, !231, !100}
!231 = !DILocation(line: 297, column: 2, scope: !9)
!232 = !DILocation(line: 241, column: 24, scope: !233)
!233 = distinct !DILexicalBlock(scope: !165, file: !1, line: 241, column: 7)
!234 = !DILocation(line: 241, column: 7, scope: !165)
!235 = !DILocation(line: 245, column: 3, scope: !236)
!236 = distinct !DILexicalBlock(scope: !233, file: !1, line: 242, column: 3)
!237 = !DILocation(line: 246, column: 13, scope: !238)
!238 = distinct !DILexicalBlock(scope: !165, file: !1, line: 246, column: 7)
!239 = !DILocation(line: 246, column: 7, scope: !165)
!240 = !DILocation(line: 250, column: 3, scope: !241)
!241 = distinct !DILexicalBlock(scope: !238, file: !1, line: 247, column: 3)
!242 = !DILocation(line: 254, column: 18, scope: !165)
!243 = !DILocalVariable(name: "zRange", scope: !9, file: !1, line: 129, type: !110)
!244 = !DILocation(line: 256, column: 24, scope: !245)
!245 = distinct !DILexicalBlock(scope: !165, file: !1, line: 256, column: 7)
!246 = !DILocation(line: 256, column: 29, scope: !245)
!247 = !DILocation(line: 256, column: 49, scope: !245)
!248 = !DILocation(line: 256, column: 7, scope: !165)
!249 = !DILocation(line: 258, column: 25, scope: !250)
!250 = distinct !DILexicalBlock(scope: !245, file: !1, line: 257, column: 3)
!251 = !DILocation(line: 258, column: 34, scope: !250)
!252 = !DILocalVariable(name: "temp_z", scope: !250, file: !1, line: 264, type: !110)
!253 = !DILocation(line: 0, scope: !250)
!254 = !DILocation(line: 269, column: 3, scope: !250)
!255 = !DILocation(line: 274, column: 24, scope: !256)
!256 = distinct !DILexicalBlock(scope: !165, file: !1, line: 274, column: 7)
!257 = !DILocation(line: 275, column: 8, scope: !256)
!258 = !DILocation(line: 275, column: 23, scope: !256)
!259 = !DILocation(line: 276, column: 4, scope: !256)
!260 = !DILocation(line: 276, column: 17, scope: !256)
!261 = !DILocation(line: 276, column: 7, scope: !256)
!262 = !DILocation(line: 276, column: 37, scope: !256)
!263 = !DILocation(line: 277, column: 4, scope: !256)
!264 = !DILocation(line: 277, column: 14, scope: !256)
!265 = !DILocation(line: 277, column: 18, scope: !256)
!266 = !DILocation(line: 277, column: 28, scope: !256)
!267 = !DILocation(line: 274, column: 7, scope: !165)
!268 = !DILocation(line: 279, column: 9, scope: !269)
!269 = distinct !DILexicalBlock(scope: !256, file: !1, line: 278, column: 3)
!270 = !DILocation(line: 283, column: 3, scope: !269)
!271 = !DILocation(line: 285, column: 17, scope: !272)
!272 = distinct !DILexicalBlock(scope: !165, file: !1, line: 285, column: 7)
!273 = !DILocation(line: 285, column: 37, scope: !272)
!274 = !DILocation(line: 285, column: 41, scope: !272)
!275 = !DILocation(line: 285, column: 50, scope: !272)
!276 = !DILocation(line: 285, column: 7, scope: !165)
!277 = !DILocation(line: 292, column: 11, scope: !278)
!278 = distinct !DILexicalBlock(scope: !272, file: !1, line: 286, column: 3)
!279 = !DILocation(line: 294, column: 3, scope: !278)
!280 = !DILocation(line: 296, column: 19, scope: !165)
!281 = !DILocation(line: 299, column: 2, scope: !9)
!282 = !DILocation(line: 300, column: 2, scope: !9)
!283 = !DILocation(line: 302, column: 2, scope: !9)
!284 = !DILocation(line: 304, column: 2, scope: !9)
