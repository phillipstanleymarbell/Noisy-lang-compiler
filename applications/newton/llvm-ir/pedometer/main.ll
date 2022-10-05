; ModuleID = 'main.ll'
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
@stderr = external dso_local global %struct._IO_FILE*, align 8
@.str.1 = private unnamed_addr constant [27 x i8] c"Error opening input file.\0A\00", align 1
@.str.2 = private unnamed_addr constant [17 x i8] c"%G,%G,%G,%G,%*G\0A\00", align 1
@.str.3 = private unnamed_addr constant [62 x i8] c"Unable to read row. Possible problem with input data format.\0A\00", align 1
@.str.4 = private unnamed_addr constant [5 x i8] c"%lf\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 %0, i8** %1) #0 !dbg !9 {
  %3 = alloca [4 x float], align 16
  %4 = alloca [4 x float], align 16
  %5 = alloca [4 x float], align 16
  %6 = alloca [4 x float], align 16
  %7 = alloca %struct.timeval, align 8
  call void @llvm.dbg.value(metadata i32 %0, metadata !16, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i8** %1, metadata !18, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata [150 x i8]* undef, metadata !19, metadata !DIExpression()), !dbg !23
  %8 = getelementptr inbounds i8*, i8** %1, i64 1, !dbg !24
  %9 = load i8*, i8** %8, align 8, !dbg !24
  %10 = call %struct._IO_FILE* @fopen(i8* %9, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i64 0, i64 0)), !dbg !25
  call void @llvm.dbg.value(metadata %struct._IO_FILE* %10, metadata !26, metadata !DIExpression()), !dbg !17
  %11 = icmp eq %struct._IO_FILE* %10, null, !dbg !86
  br i1 %11, label %12, label %15, !dbg !88

12:                                               ; preds = %2
  %13 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 8, !dbg !89
  %14 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %13, i8* getelementptr inbounds ([27 x i8], [27 x i8]* @.str.1, i64 0, i64 0)), !dbg !91
  call void @exit(i32 1) #5, !dbg !92
  unreachable, !dbg !92

15:                                               ; preds = %2
  br label %16, !dbg !93

16:                                               ; preds = %19, %15
  %17 = call i32 @fgetc(%struct._IO_FILE* %10), !dbg !94
  %18 = icmp ne i32 %17, 10, !dbg !95
  br i1 %18, label %19, label %20, !dbg !93

19:                                               ; preds = %16
  br label %16, !dbg !93, !llvm.loop !96

20:                                               ; preds = %16
  call void @llvm.dbg.value(metadata i32 0, metadata !99, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 -1, metadata !100, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !101, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 1, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !103, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !104, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 1, metadata !105, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !106, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata float* undef, metadata !108, metadata !DIExpression()), !dbg !109
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !110, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !111, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !112, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float -1.000000e+03, metadata !113, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata float* undef, metadata !114, metadata !DIExpression()), !dbg !115
  call void @llvm.dbg.declare(metadata [4 x float]* %3, metadata !116, metadata !DIExpression()), !dbg !120
  call void @llvm.dbg.declare(metadata [4 x float]* %4, metadata !121, metadata !DIExpression()), !dbg !122
  call void @llvm.dbg.declare(metadata [4 x float]* %5, metadata !123, metadata !DIExpression()), !dbg !124
  call void @llvm.dbg.declare(metadata [4 x float]* %6, metadata !125, metadata !DIExpression()), !dbg !126
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !127, metadata !DIExpression()), !dbg !131
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !132, metadata !DIExpression()), !dbg !133
  call void @llvm.dbg.declare(metadata [48 x float]* undef, metadata !134, metadata !DIExpression()), !dbg !135
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !136, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !137, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float -1.000000e+04, metadata !138, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !139, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !140, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 1.000000e+04, metadata !141, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !142, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !143, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !144, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !145, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !147, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !149, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !151, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !152, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !153, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !154, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !155, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !156, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata %struct.timeval* %7, metadata !157, metadata !DIExpression()), !dbg !165
  %21 = call i32 @gettimeofday(%struct.timeval* %7, %struct.timezone* null) #6, !dbg !166
  %22 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 0, !dbg !167
  %23 = load i64, i64* %22, align 8, !dbg !167
  %24 = sitofp i64 %23 to double, !dbg !168
  %25 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 1, !dbg !169
  %26 = load i64, i64* %25, align 8, !dbg !169
  %27 = sitofp i64 %26 to double, !dbg !170
  %28 = fmul double 0x3EB0C6F7A0B5ED8D, %27, !dbg !171
  %29 = fadd double %24, %28, !dbg !172
  call void @llvm.dbg.value(metadata double %29, metadata !173, metadata !DIExpression()), !dbg !17
  br label %30, !dbg !175

30:                                               ; preds = %114, %77, %20
  %.026 = phi float [ -1.000000e+03, %20 ], [ %.127, %114 ], [ %.026, %77 ], !dbg !17
  %.024 = phi float [ undef, %20 ], [ %.125, %114 ], [ %.125, %77 ]
  %.019 = phi float [ -1.000000e+04, %20 ], [ %.221, %114 ], [ %.019, %77 ], !dbg !17
  %.016 = phi float [ 1.000000e+04, %20 ], [ %.218, %114 ], [ %.016, %77 ], !dbg !17
  %.014 = phi float [ 0.000000e+00, %20 ], [ %.115, %114 ], [ %.014, %77 ], !dbg !17
  %.012 = phi i32 [ 0, %20 ], [ %.113, %114 ], [ %.012, %77 ], !dbg !17
  %.010 = phi i32 [ 0, %20 ], [ %.111, %114 ], [ %.010, %77 ], !dbg !17
  %.08 = phi i32 [ 1, %20 ], [ %.2, %114 ], [ %.08, %77 ], !dbg !17
  %.06 = phi i32 [ -1, %20 ], [ %115, %114 ], [ %.17, %77 ], !dbg !17
  %.04 = phi i32 [ 0, %20 ], [ %.15, %114 ], [ %.15, %77 ], !dbg !17
  %.03 = phi float [ 0.000000e+00, %20 ], [ %.023, %114 ], [ %.1, %77 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.03, metadata !153, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.04, metadata !99, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.06, metadata !100, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.08, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.010, metadata !103, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.012, metadata !104, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.014, metadata !144, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.016, metadata !141, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.019, metadata !138, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.024, metadata !176, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.026, metadata !113, metadata !DIExpression()), !dbg !17
  %31 = icmp ne i32 %.04, 0, !dbg !177
  %32 = xor i1 %31, true, !dbg !177
  br i1 %32, label %33, label %116, !dbg !175

33:                                               ; preds = %30
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !178, metadata !DIExpression()), !dbg !180
  call void @llvm.dbg.value(metadata i32 0, metadata !181, metadata !DIExpression()), !dbg !180
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !182, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !183, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !184, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i64 0, metadata !185, metadata !DIExpression()), !dbg !187
  br label %34, !dbg !188

34:                                               ; preds = %66, %33
  %.028 = phi float [ 0.000000e+00, %33 ], [ %57, %66 ], !dbg !180
  %.125 = phi float [ %.024, %33 ], [ %65, %66 ]
  %.023 = phi float [ 0.000000e+00, %33 ], [ %60, %66 ], !dbg !180
  %.022 = phi float [ 0.000000e+00, %33 ], [ %63, %66 ], !dbg !180
  %.02 = phi float [ 0.000000e+00, %33 ], [ %53, %66 ], !dbg !180
  %.01 = phi i32 [ 0, %33 ], [ %54, %66 ], !dbg !180
  %.0 = phi i64 [ 0, %33 ], [ %67, %66 ], !dbg !187
  call void @llvm.dbg.value(metadata i64 %.0, metadata !185, metadata !DIExpression()), !dbg !187
  call void @llvm.dbg.value(metadata i32 %.01, metadata !181, metadata !DIExpression()), !dbg !180
  call void @llvm.dbg.value(metadata float %.02, metadata !178, metadata !DIExpression()), !dbg !180
  call void @llvm.dbg.value(metadata float %.022, metadata !184, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.023, metadata !183, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.125, metadata !176, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.028, metadata !182, metadata !DIExpression()), !dbg !17
  %35 = icmp ult i64 %.0, 4, !dbg !189
  br i1 %35, label %36, label %68, !dbg !191

36:                                               ; preds = %34
  %37 = getelementptr inbounds [4 x float], [4 x float]* %3, i64 0, i64 %.0, !dbg !192
  %38 = getelementptr inbounds [4 x float], [4 x float]* %4, i64 0, i64 %.0, !dbg !194
  %39 = getelementptr inbounds [4 x float], [4 x float]* %5, i64 0, i64 %.0, !dbg !195
  %40 = getelementptr inbounds [4 x float], [4 x float]* %6, i64 0, i64 %.0, !dbg !196
  %41 = call i32 (%struct._IO_FILE*, i8*, ...) @__isoc99_fscanf(%struct._IO_FILE* %10, i8* getelementptr inbounds ([17 x i8], [17 x i8]* @.str.2, i64 0, i64 0), float* %37, float* %38, float* %39, float* %40), !dbg !197
  call void @llvm.dbg.value(metadata i32 %41, metadata !198, metadata !DIExpression()), !dbg !199
  %42 = icmp eq i32 %41, -1, !dbg !200
  br i1 %42, label %45, label %43, !dbg !202

43:                                               ; preds = %36
  %44 = icmp eq i32 %41, 0, !dbg !203
  br i1 %44, label %45, label %46, !dbg !204

45:                                               ; preds = %43, %36
  call void @llvm.dbg.value(metadata i32 1, metadata !99, metadata !DIExpression()), !dbg !17
  br label %68, !dbg !205

46:                                               ; preds = %43
  %47 = icmp ne i32 %41, 4, !dbg !207
  br i1 %47, label %48, label %51, !dbg !209

48:                                               ; preds = %46
  %49 = load %struct._IO_FILE*, %struct._IO_FILE** @stderr, align 8, !dbg !210
  %50 = call i32 (%struct._IO_FILE*, i8*, ...) @fprintf(%struct._IO_FILE* %49, i8* getelementptr inbounds ([62 x i8], [62 x i8]* @.str.3, i64 0, i64 0)), !dbg !212
  br label %51, !dbg !213

51:                                               ; preds = %48, %46
  br label %52

52:                                               ; preds = %51
  %53 = fadd float %.02, %.125, !dbg !214
  call void @llvm.dbg.value(metadata float %53, metadata !178, metadata !DIExpression()), !dbg !180
  %54 = add nsw i32 %.01, 1, !dbg !215
  call void @llvm.dbg.value(metadata i32 %54, metadata !181, metadata !DIExpression()), !dbg !180
  %55 = getelementptr inbounds [4 x float], [4 x float]* %4, i64 0, i64 %.0, !dbg !216
  %56 = load float, float* %55, align 4, !dbg !216
  %57 = fadd float %.028, %56, !dbg !217
  call void @llvm.dbg.value(metadata float %57, metadata !182, metadata !DIExpression()), !dbg !17
  %58 = getelementptr inbounds [4 x float], [4 x float]* %5, i64 0, i64 %.0, !dbg !218
  %59 = load float, float* %58, align 4, !dbg !218
  %60 = fadd float %.023, %59, !dbg !219
  call void @llvm.dbg.value(metadata float %60, metadata !183, metadata !DIExpression()), !dbg !17
  %61 = getelementptr inbounds [4 x float], [4 x float]* %6, i64 0, i64 %.0, !dbg !220
  %62 = load float, float* %61, align 4, !dbg !220
  %63 = fadd float %.022, %62, !dbg !221
  call void @llvm.dbg.value(metadata float %63, metadata !184, metadata !DIExpression()), !dbg !17
  %64 = getelementptr inbounds [4 x float], [4 x float]* %3, i64 0, i64 %.0, !dbg !222
  %65 = load float, float* %64, align 4, !dbg !222
  call void @llvm.dbg.value(metadata float %65, metadata !176, metadata !DIExpression()), !dbg !17
  br label %66, !dbg !223

66:                                               ; preds = %52
  %67 = add i64 %.0, 1, !dbg !224
  call void @llvm.dbg.value(metadata i64 %67, metadata !185, metadata !DIExpression()), !dbg !187
  br label %34, !dbg !225, !llvm.loop !226

68:                                               ; preds = %45, %34
  %.15 = phi i32 [ 1, %45 ], [ %.04, %34 ], !dbg !17
  call void @llvm.dbg.value(metadata i32 %.15, metadata !99, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata float* undef, metadata !228, metadata !DIExpression()), !dbg !229
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !145, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float 0.000000e+00, metadata !147, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.03, metadata !149, metadata !DIExpression()), !dbg !17
  %69 = icmp eq i32 %.06, -1, !dbg !230
  br i1 %69, label %70, label %71, !dbg !232

70:                                               ; preds = %68
  call void @llvm.dbg.value(metadata float %.023, metadata !153, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !100, metadata !DIExpression()), !dbg !17
  br label %71, !dbg !233

71:                                               ; preds = %70, %68
  %.17 = phi i32 [ 0, %70 ], [ %.06, %68 ], !dbg !17
  %.1 = phi float [ %.023, %70 ], [ %.03, %68 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.1, metadata !153, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.17, metadata !100, metadata !DIExpression()), !dbg !17
  %72 = fsub float %.023, %.1, !dbg !235
  %73 = call float @llvm.fabs.f32(float %72), !dbg !237
  %74 = fpext float %73 to double, !dbg !237
  %75 = fcmp ogt double %74, 5.000000e-01, !dbg !238
  br i1 %75, label %76, label %77, !dbg !239

76:                                               ; preds = %71
  call void @llvm.dbg.value(metadata float %.023, metadata !153, metadata !DIExpression()), !dbg !17
  br label %78, !dbg !240

77:                                               ; preds = %71
  br label %30, !dbg !242, !llvm.loop !244

78:                                               ; preds = %76
  %79 = fcmp ogt float %.023, %.019, !dbg !246
  br i1 %79, label %80, label %81, !dbg !248

80:                                               ; preds = %78
  call void @llvm.dbg.value(metadata float %.023, metadata !138, metadata !DIExpression()), !dbg !17
  br label %81, !dbg !249

81:                                               ; preds = %80, %78
  %.120 = phi float [ %.023, %80 ], [ %.019, %78 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.120, metadata !138, metadata !DIExpression()), !dbg !17
  %82 = fcmp ogt float %.016, %.023, !dbg !251
  br i1 %82, label %83, label %84, !dbg !253

83:                                               ; preds = %81
  call void @llvm.dbg.value(metadata float %.023, metadata !141, metadata !DIExpression()), !dbg !17
  br label %84, !dbg !254

84:                                               ; preds = %83, %81
  %.117 = phi float [ %.023, %83 ], [ %.016, %81 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.117, metadata !141, metadata !DIExpression()), !dbg !17
  %85 = fsub float %.120, %.117, !dbg !256
  call void @llvm.dbg.value(metadata float %85, metadata !257, metadata !DIExpression()), !dbg !17
  %86 = icmp slt i32 %.17, 10, !dbg !258
  br i1 %86, label %90, label %87, !dbg !260

87:                                               ; preds = %84
  %88 = srem i32 %.17, 50, !dbg !261
  %89 = icmp ne i32 %88, 0, !dbg !261
  br i1 %89, label %90, label %93, !dbg !262

90:                                               ; preds = %87, %84
  %91 = fadd float %.120, %.117, !dbg !263
  %92 = fdiv float %91, 2.000000e+00, !dbg !265
  call void @llvm.dbg.value(metadata float %92, metadata !144, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.120, metadata !266, metadata !DIExpression()), !dbg !267
  call void @llvm.dbg.value(metadata float %.117, metadata !138, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.120, metadata !141, metadata !DIExpression()), !dbg !17
  br label %93, !dbg !268

93:                                               ; preds = %90, %87
  %.221 = phi float [ %.117, %90 ], [ %.120, %87 ], !dbg !180
  %.218 = phi float [ %.120, %90 ], [ %.117, %87 ], !dbg !180
  %.115 = phi float [ %92, %90 ], [ %.014, %87 ], !dbg !17
  call void @llvm.dbg.value(metadata float %.115, metadata !144, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.218, metadata !141, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.221, metadata !138, metadata !DIExpression()), !dbg !17
  %94 = fcmp ogt float %.03, %.023, !dbg !269
  br i1 %94, label %95, label %107, !dbg !271

95:                                               ; preds = %93
  %96 = fcmp ogt float %.115, %.023, !dbg !272
  br i1 %96, label %97, label %107, !dbg !273

97:                                               ; preds = %95
  %98 = fsub float %.125, %.026, !dbg !274
  %99 = fpext float %98 to double, !dbg !275
  %100 = fcmp ogt double %99, 3.000000e-01, !dbg !276
  br i1 %100, label %101, label %107, !dbg !277

101:                                              ; preds = %97
  %102 = fcmp ogt float %85, 2.000000e+00, !dbg !278
  br i1 %102, label %103, label %107, !dbg !279

103:                                              ; preds = %101
  %104 = fcmp olt float %85, 8.000000e+00, !dbg !280
  br i1 %104, label %105, label %107, !dbg !281

105:                                              ; preds = %103
  %106 = add nsw i32 %.010, 1, !dbg !282
  call void @llvm.dbg.value(metadata i32 %106, metadata !103, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 0, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.125, metadata !113, metadata !DIExpression()), !dbg !17
  br label %107, !dbg !284

107:                                              ; preds = %105, %103, %101, %97, %95, %93
  %.127 = phi float [ %.125, %105 ], [ %.026, %103 ], [ %.026, %101 ], [ %.026, %97 ], [ %.026, %95 ], [ %.026, %93 ], !dbg !17
  %.111 = phi i32 [ %106, %105 ], [ %.010, %103 ], [ %.010, %101 ], [ %.010, %97 ], [ %.010, %95 ], [ %.010, %93 ], !dbg !17
  %.19 = phi i32 [ 0, %105 ], [ %.08, %103 ], [ %.08, %101 ], [ %.08, %97 ], [ %.08, %95 ], [ %.08, %93 ], !dbg !17
  call void @llvm.dbg.value(metadata i32 %.19, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.111, metadata !103, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata float %.127, metadata !113, metadata !DIExpression()), !dbg !17
  %108 = fsub float %.125, %.127, !dbg !285
  %109 = fcmp ogt float %108, 2.000000e+00, !dbg !287
  br i1 %109, label %110, label %114, !dbg !288

110:                                              ; preds = %107
  %111 = icmp eq i32 %.19, 0, !dbg !289
  br i1 %111, label %112, label %114, !dbg !290

112:                                              ; preds = %110
  %113 = add nsw i32 %.012, 1, !dbg !291
  call void @llvm.dbg.value(metadata i32 %113, metadata !104, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 1, metadata !102, metadata !DIExpression()), !dbg !17
  br label %114, !dbg !293

114:                                              ; preds = %112, %110, %107
  %.113 = phi i32 [ %113, %112 ], [ %.012, %110 ], [ %.012, %107 ], !dbg !17
  %.2 = phi i32 [ 1, %112 ], [ %.19, %110 ], [ %.19, %107 ], !dbg !180
  call void @llvm.dbg.value(metadata i32 %.2, metadata !102, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.value(metadata i32 %.113, metadata !104, metadata !DIExpression()), !dbg !17
  %115 = add nsw i32 %.17, 1, !dbg !294
  call void @llvm.dbg.value(metadata i32 %115, metadata !100, metadata !DIExpression()), !dbg !17
  br label %30, !dbg !175, !llvm.loop !244

116:                                              ; preds = %30
  %117 = call i32 @gettimeofday(%struct.timeval* %7, %struct.timezone* null) #6, !dbg !295
  %118 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 0, !dbg !296
  %119 = load i64, i64* %118, align 8, !dbg !296
  %120 = sitofp i64 %119 to double, !dbg !297
  %121 = getelementptr inbounds %struct.timeval, %struct.timeval* %7, i32 0, i32 1, !dbg !298
  %122 = load i64, i64* %121, align 8, !dbg !298
  %123 = sitofp i64 %122 to double, !dbg !299
  %124 = fmul double 0x3EB0C6F7A0B5ED8D, %123, !dbg !300
  %125 = fadd double %120, %124, !dbg !301
  call void @llvm.dbg.value(metadata double %125, metadata !302, metadata !DIExpression()), !dbg !17
  %126 = fsub double %125, %29, !dbg !303
  %127 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.4, i64 0, i64 0), double %126), !dbg !304
  %128 = call i32 @fclose(%struct._IO_FILE* %10), !dbg !305
  ret i32 0, !dbg !306
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare dso_local %struct._IO_FILE* @fopen(i8*, i8*) #2

declare dso_local i32 @fprintf(%struct._IO_FILE*, i8*, ...) #2

; Function Attrs: noreturn nounwind
declare dso_local void @exit(i32) #3

declare dso_local i32 @fgetc(%struct._IO_FILE*) #2

; Function Attrs: nounwind
declare dso_local i32 @gettimeofday(%struct.timeval*, %struct.timezone*) #4

declare dso_local i32 @__isoc99_fscanf(%struct._IO_FILE*, i8*, ...) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.fabs.f32(float) #1

declare dso_local i32 @printf(i8*, ...) #2

declare dso_local i32 @fclose(%struct._IO_FILE*) #2

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind uwtable "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noreturn nounwind "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { noreturn nounwind }
attributes #6 = { nounwind }

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
!9 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 50, type: !10, scopeLine: 51, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !2)
!10 = !DISubroutineType(types: !11)
!11 = !{!12, !12, !13}
!12 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!13 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !14, size: 64)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!16 = !DILocalVariable(name: "argc", arg: 1, scope: !9, file: !1, line: 50, type: !12)
!17 = !DILocation(line: 0, scope: !9)
!18 = !DILocalVariable(name: "argv", arg: 2, scope: !9, file: !1, line: 50, type: !13)
!19 = !DILocalVariable(name: "charBuffer", scope: !9, file: !1, line: 52, type: !20)
!20 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 1200, elements: !21)
!21 = !{!22}
!22 = !DISubrange(count: 150)
!23 = !DILocation(line: 52, column: 7, scope: !9)
!24 = !DILocation(line: 57, column: 20, scope: !9)
!25 = !DILocation(line: 57, column: 14, scope: !9)
!26 = !DILocalVariable(name: "inputFile", scope: !9, file: !1, line: 53, type: !27)
!27 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !28, size: 64)
!28 = !DIDerivedType(tag: DW_TAG_typedef, name: "FILE", file: !29, line: 7, baseType: !30)
!29 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/FILE.h", directory: "")
!30 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_FILE", file: !31, line: 49, size: 1728, elements: !32)
!31 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h", directory: "")
!32 = !{!33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !48, !50, !51, !52, !56, !58, !60, !64, !67, !69, !72, !75, !76, !77, !81, !82}
!33 = !DIDerivedType(tag: DW_TAG_member, name: "_flags", scope: !30, file: !31, line: 51, baseType: !12, size: 32)
!34 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_ptr", scope: !30, file: !31, line: 54, baseType: !14, size: 64, offset: 64)
!35 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_end", scope: !30, file: !31, line: 55, baseType: !14, size: 64, offset: 128)
!36 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_read_base", scope: !30, file: !31, line: 56, baseType: !14, size: 64, offset: 192)
!37 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_base", scope: !30, file: !31, line: 57, baseType: !14, size: 64, offset: 256)
!38 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_ptr", scope: !30, file: !31, line: 58, baseType: !14, size: 64, offset: 320)
!39 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_write_end", scope: !30, file: !31, line: 59, baseType: !14, size: 64, offset: 384)
!40 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_base", scope: !30, file: !31, line: 60, baseType: !14, size: 64, offset: 448)
!41 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_buf_end", scope: !30, file: !31, line: 61, baseType: !14, size: 64, offset: 512)
!42 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_base", scope: !30, file: !31, line: 64, baseType: !14, size: 64, offset: 576)
!43 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_backup_base", scope: !30, file: !31, line: 65, baseType: !14, size: 64, offset: 640)
!44 = !DIDerivedType(tag: DW_TAG_member, name: "_IO_save_end", scope: !30, file: !31, line: 66, baseType: !14, size: 64, offset: 704)
!45 = !DIDerivedType(tag: DW_TAG_member, name: "_markers", scope: !30, file: !31, line: 68, baseType: !46, size: 64, offset: 768)
!46 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !47, size: 64)
!47 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_marker", file: !31, line: 36, flags: DIFlagFwdDecl)
!48 = !DIDerivedType(tag: DW_TAG_member, name: "_chain", scope: !30, file: !31, line: 70, baseType: !49, size: 64, offset: 832)
!49 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !30, size: 64)
!50 = !DIDerivedType(tag: DW_TAG_member, name: "_fileno", scope: !30, file: !31, line: 72, baseType: !12, size: 32, offset: 896)
!51 = !DIDerivedType(tag: DW_TAG_member, name: "_flags2", scope: !30, file: !31, line: 73, baseType: !12, size: 32, offset: 928)
!52 = !DIDerivedType(tag: DW_TAG_member, name: "_old_offset", scope: !30, file: !31, line: 74, baseType: !53, size: 64, offset: 960)
!53 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off_t", file: !54, line: 152, baseType: !55)
!54 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types.h", directory: "")
!55 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!56 = !DIDerivedType(tag: DW_TAG_member, name: "_cur_column", scope: !30, file: !31, line: 77, baseType: !57, size: 16, offset: 1024)
!57 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!58 = !DIDerivedType(tag: DW_TAG_member, name: "_vtable_offset", scope: !30, file: !31, line: 78, baseType: !59, size: 8, offset: 1040)
!59 = !DIBasicType(name: "signed char", size: 8, encoding: DW_ATE_signed_char)
!60 = !DIDerivedType(tag: DW_TAG_member, name: "_shortbuf", scope: !30, file: !31, line: 79, baseType: !61, size: 8, offset: 1048)
!61 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 8, elements: !62)
!62 = !{!63}
!63 = !DISubrange(count: 1)
!64 = !DIDerivedType(tag: DW_TAG_member, name: "_lock", scope: !30, file: !31, line: 81, baseType: !65, size: 64, offset: 1088)
!65 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !66, size: 64)
!66 = !DIDerivedType(tag: DW_TAG_typedef, name: "_IO_lock_t", file: !31, line: 43, baseType: null)
!67 = !DIDerivedType(tag: DW_TAG_member, name: "_offset", scope: !30, file: !31, line: 89, baseType: !68, size: 64, offset: 1152)
!68 = !DIDerivedType(tag: DW_TAG_typedef, name: "__off64_t", file: !54, line: 153, baseType: !55)
!69 = !DIDerivedType(tag: DW_TAG_member, name: "_codecvt", scope: !30, file: !31, line: 91, baseType: !70, size: 64, offset: 1216)
!70 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !71, size: 64)
!71 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_codecvt", file: !31, line: 37, flags: DIFlagFwdDecl)
!72 = !DIDerivedType(tag: DW_TAG_member, name: "_wide_data", scope: !30, file: !31, line: 92, baseType: !73, size: 64, offset: 1280)
!73 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !74, size: 64)
!74 = !DICompositeType(tag: DW_TAG_structure_type, name: "_IO_wide_data", file: !31, line: 38, flags: DIFlagFwdDecl)
!75 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_list", scope: !30, file: !31, line: 93, baseType: !49, size: 64, offset: 1344)
!76 = !DIDerivedType(tag: DW_TAG_member, name: "_freeres_buf", scope: !30, file: !31, line: 94, baseType: !4, size: 64, offset: 1408)
!77 = !DIDerivedType(tag: DW_TAG_member, name: "__pad5", scope: !30, file: !31, line: 95, baseType: !78, size: 64, offset: 1472)
!78 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !79, line: 46, baseType: !80)
!79 = !DIFile(filename: "/usr/lib/llvm-12/lib/clang/12.0.1/include/stddef.h", directory: "")
!80 = !DIBasicType(name: "long unsigned int", size: 64, encoding: DW_ATE_unsigned)
!81 = !DIDerivedType(tag: DW_TAG_member, name: "_mode", scope: !30, file: !31, line: 96, baseType: !12, size: 32, offset: 1536)
!82 = !DIDerivedType(tag: DW_TAG_member, name: "_unused2", scope: !30, file: !31, line: 98, baseType: !83, size: 160, offset: 1568)
!83 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 160, elements: !84)
!84 = !{!85}
!85 = !DISubrange(count: 20)
!86 = !DILocation(line: 58, column: 16, scope: !87)
!87 = distinct !DILexicalBlock(scope: !9, file: !1, line: 58, column: 6)
!88 = !DILocation(line: 58, column: 6, scope: !9)
!89 = !DILocation(line: 60, column: 11, scope: !90)
!90 = distinct !DILexicalBlock(scope: !87, file: !1, line: 59, column: 2)
!91 = !DILocation(line: 60, column: 3, scope: !90)
!92 = !DILocation(line: 61, column: 3, scope: !90)
!93 = !DILocation(line: 67, column: 2, scope: !9)
!94 = !DILocation(line: 67, column: 9, scope: !9)
!95 = !DILocation(line: 67, column: 26, scope: !9)
!96 = distinct !{!96, !93, !97, !98}
!97 = !DILocation(line: 67, column: 34, scope: !9)
!98 = !{!"llvm.loop.mustprogress"}
!99 = !DILocalVariable(name: "reachedEOF", scope: !9, file: !1, line: 70, type: !12)
!100 = !DILocalVariable(name: "measurementCount", scope: !9, file: !1, line: 72, type: !12)
!101 = !DILocalVariable(name: "DCEstimateCounter", scope: !9, file: !1, line: 73, type: !12)
!102 = !DILocalVariable(name: "reset", scope: !9, file: !1, line: 75, type: !12)
!103 = !DILocalVariable(name: "steps", scope: !9, file: !1, line: 76, type: !12)
!104 = !DILocalVariable(name: "strides", scope: !9, file: !1, line: 77, type: !12)
!105 = !DILocalVariable(name: "inStride", scope: !9, file: !1, line: 78, type: !12)
!106 = !DILocalVariable(name: "stepProbability", scope: !9, file: !1, line: 80, type: !107)
!107 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!108 = !DILocalVariable(name: "stepIntersectionProbability", scope: !9, file: !1, line: 81, type: !107)
!109 = !DILocation(line: 81, column: 8, scope: !9)
!110 = !DILocalVariable(name: "fSteps", scope: !9, file: !1, line: 82, type: !107)
!111 = !DILocalVariable(name: "bernoulliSum", scope: !9, file: !1, line: 83, type: !107)
!112 = !DILocalVariable(name: "poissonBinomialAccum", scope: !9, file: !1, line: 84, type: !107)
!113 = !DILocalVariable(name: "timestampPrevious", scope: !9, file: !1, line: 87, type: !107)
!114 = !DILocalVariable(name: "fTimestampPrevious", scope: !9, file: !1, line: 88, type: !107)
!115 = !DILocation(line: 88, column: 8, scope: !9)
!116 = !DILocalVariable(name: "timestampSamples", scope: !9, file: !1, line: 93, type: !117)
!117 = !DICompositeType(tag: DW_TAG_array_type, baseType: !107, size: 128, elements: !118)
!118 = !{!119}
!119 = !DISubrange(count: 4)
!120 = !DILocation(line: 93, column: 8, scope: !9)
!121 = !DILocalVariable(name: "xSamples", scope: !9, file: !1, line: 94, type: !117)
!122 = !DILocation(line: 94, column: 8, scope: !9)
!123 = !DILocalVariable(name: "ySamples", scope: !9, file: !1, line: 95, type: !117)
!124 = !DILocation(line: 95, column: 8, scope: !9)
!125 = !DILocalVariable(name: "zSamples", scope: !9, file: !1, line: 96, type: !117)
!126 = !DILocation(line: 96, column: 8, scope: !9)
!127 = !DILocalVariable(name: "xDCSamples", scope: !9, file: !1, line: 97, type: !128)
!128 = !DICompositeType(tag: DW_TAG_array_type, baseType: !107, size: 1536, elements: !129)
!129 = !{!130}
!130 = !DISubrange(count: 48)
!131 = !DILocation(line: 97, column: 8, scope: !9)
!132 = !DILocalVariable(name: "yDCSamples", scope: !9, file: !1, line: 98, type: !128)
!133 = !DILocation(line: 98, column: 8, scope: !9)
!134 = !DILocalVariable(name: "zDCSamples", scope: !9, file: !1, line: 99, type: !128)
!135 = !DILocation(line: 99, column: 8, scope: !9)
!136 = !DILocalVariable(name: "max_x", scope: !9, file: !1, line: 109, type: !107)
!137 = !DILocalVariable(name: "max_y", scope: !9, file: !1, line: 110, type: !107)
!138 = !DILocalVariable(name: "max_z", scope: !9, file: !1, line: 111, type: !107)
!139 = !DILocalVariable(name: "min_x", scope: !9, file: !1, line: 112, type: !107)
!140 = !DILocalVariable(name: "min_y", scope: !9, file: !1, line: 113, type: !107)
!141 = !DILocalVariable(name: "min_z", scope: !9, file: !1, line: 114, type: !107)
!142 = !DILocalVariable(name: "threshold_x", scope: !9, file: !1, line: 115, type: !107)
!143 = !DILocalVariable(name: "threshold_y", scope: !9, file: !1, line: 116, type: !107)
!144 = !DILocalVariable(name: "threshold_z", scope: !9, file: !1, line: 117, type: !107)
!145 = !DILocalVariable(name: "measurementOld_x", scope: !9, file: !1, line: 119, type: !146)
!146 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055xAcceleration", scope: !9, file: !1, line: 101, baseType: !107)
!147 = !DILocalVariable(name: "measurementOld_y", scope: !9, file: !1, line: 120, type: !148)
!148 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055yAcceleration", scope: !9, file: !1, line: 102, baseType: !107)
!149 = !DILocalVariable(name: "measurementOld_z", scope: !9, file: !1, line: 121, type: !150)
!150 = !DIDerivedType(tag: DW_TAG_typedef, name: "bmx055zAcceleration", scope: !9, file: !1, line: 103, baseType: !107)
!151 = !DILocalVariable(name: "measurementNew_x", scope: !9, file: !1, line: 122, type: !146)
!152 = !DILocalVariable(name: "measurementNew_y", scope: !9, file: !1, line: 123, type: !148)
!153 = !DILocalVariable(name: "measurementNew_z", scope: !9, file: !1, line: 124, type: !150)
!154 = !DILocalVariable(name: "xDC", scope: !9, file: !1, line: 126, type: !107)
!155 = !DILocalVariable(name: "yDC", scope: !9, file: !1, line: 127, type: !107)
!156 = !DILocalVariable(name: "zDC", scope: !9, file: !1, line: 128, type: !107)
!157 = !DILocalVariable(name: "tv", scope: !9, file: !1, line: 134, type: !158)
!158 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "timeval", file: !159, line: 8, size: 128, elements: !160)
!159 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types/struct_timeval.h", directory: "")
!160 = !{!161, !163}
!161 = !DIDerivedType(tag: DW_TAG_member, name: "tv_sec", scope: !158, file: !159, line: 10, baseType: !162, size: 64)
!162 = !DIDerivedType(tag: DW_TAG_typedef, name: "__time_t", file: !54, line: 160, baseType: !55)
!163 = !DIDerivedType(tag: DW_TAG_member, name: "tv_usec", scope: !158, file: !159, line: 11, baseType: !164, size: 64, offset: 64)
!164 = !DIDerivedType(tag: DW_TAG_typedef, name: "__suseconds_t", file: !54, line: 162, baseType: !55)
!165 = !DILocation(line: 134, column: 17, scope: !9)
!166 = !DILocation(line: 135, column: 5, scope: !9)
!167 = !DILocation(line: 136, column: 20, scope: !9)
!168 = !DILocation(line: 136, column: 17, scope: !9)
!169 = !DILocation(line: 136, column: 41, scope: !9)
!170 = !DILocation(line: 136, column: 38, scope: !9)
!171 = !DILocation(line: 136, column: 36, scope: !9)
!172 = !DILocation(line: 136, column: 27, scope: !9)
!173 = !DILocalVariable(name: "t1", scope: !9, file: !1, line: 136, type: !174)
!174 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
!175 = !DILocation(line: 138, column: 2, scope: !9)
!176 = !DILocalVariable(name: "timestamp", scope: !9, file: !1, line: 86, type: !107)
!177 = !DILocation(line: 138, column: 9, scope: !9)
!178 = !DILocalVariable(name: "timestampAccum", scope: !179, file: !1, line: 147, type: !107)
!179 = distinct !DILexicalBlock(scope: !9, file: !1, line: 139, column: 2)
!180 = !DILocation(line: 0, scope: !179)
!181 = !DILocalVariable(name: "samplesTaken", scope: !179, file: !1, line: 148, type: !12)
!182 = !DILocalVariable(name: "acc_x", scope: !9, file: !1, line: 105, type: !146)
!183 = !DILocalVariable(name: "acc_y", scope: !9, file: !1, line: 106, type: !148)
!184 = !DILocalVariable(name: "acc_z", scope: !9, file: !1, line: 107, type: !150)
!185 = !DILocalVariable(name: "i", scope: !186, file: !1, line: 153, type: !78)
!186 = distinct !DILexicalBlock(scope: !179, file: !1, line: 153, column: 3)
!187 = !DILocation(line: 0, scope: !186)
!188 = !DILocation(line: 153, column: 8, scope: !186)
!189 = !DILocation(line: 153, column: 24, scope: !190)
!190 = distinct !DILexicalBlock(scope: !186, file: !1, line: 153, column: 3)
!191 = !DILocation(line: 153, column: 3, scope: !186)
!192 = !DILocation(line: 156, column: 60, scope: !193)
!193 = distinct !DILexicalBlock(scope: !190, file: !1, line: 154, column: 3)
!194 = !DILocation(line: 156, column: 82, scope: !193)
!195 = !DILocation(line: 156, column: 96, scope: !193)
!196 = !DILocation(line: 156, column: 110, scope: !193)
!197 = !DILocation(line: 156, column: 20, scope: !193)
!198 = !DILocalVariable(name: "itemsRead", scope: !193, file: !1, line: 156, type: !12)
!199 = !DILocation(line: 0, scope: !193)
!200 = !DILocation(line: 157, column: 18, scope: !201)
!201 = distinct !DILexicalBlock(scope: !193, file: !1, line: 157, column: 8)
!202 = !DILocation(line: 157, column: 25, scope: !201)
!203 = !DILocation(line: 157, column: 38, scope: !201)
!204 = !DILocation(line: 157, column: 8, scope: !193)
!205 = !DILocation(line: 163, column: 5, scope: !206)
!206 = distinct !DILexicalBlock(scope: !201, file: !1, line: 158, column: 4)
!207 = !DILocation(line: 165, column: 23, scope: !208)
!208 = distinct !DILexicalBlock(scope: !201, file: !1, line: 165, column: 13)
!209 = !DILocation(line: 165, column: 13, scope: !201)
!210 = !DILocation(line: 167, column: 13, scope: !211)
!211 = distinct !DILexicalBlock(scope: !208, file: !1, line: 166, column: 4)
!212 = !DILocation(line: 167, column: 5, scope: !211)
!213 = !DILocation(line: 168, column: 4, scope: !211)
!214 = !DILocation(line: 170, column: 19, scope: !193)
!215 = !DILocation(line: 171, column: 16, scope: !193)
!216 = !DILocation(line: 176, column: 13, scope: !193)
!217 = !DILocation(line: 176, column: 10, scope: !193)
!218 = !DILocation(line: 177, column: 13, scope: !193)
!219 = !DILocation(line: 177, column: 10, scope: !193)
!220 = !DILocation(line: 178, column: 13, scope: !193)
!221 = !DILocation(line: 178, column: 10, scope: !193)
!222 = !DILocation(line: 179, column: 16, scope: !193)
!223 = !DILocation(line: 180, column: 3, scope: !193)
!224 = !DILocation(line: 153, column: 52, scope: !190)
!225 = !DILocation(line: 153, column: 3, scope: !190)
!226 = distinct !{!226, !191, !227, !98}
!227 = !DILocation(line: 180, column: 3, scope: !186)
!228 = !DILocalVariable(name: "diff_z", scope: !179, file: !1, line: 195, type: !107)
!229 = !DILocation(line: 195, column: 9, scope: !179)
!230 = !DILocation(line: 201, column: 24, scope: !231)
!231 = distinct !DILexicalBlock(scope: !179, file: !1, line: 201, column: 7)
!232 = !DILocation(line: 201, column: 7, scope: !179)
!233 = !DILocation(line: 205, column: 3, scope: !234)
!234 = distinct !DILexicalBlock(scope: !231, file: !1, line: 202, column: 3)
!235 = !DILocation(line: 229, column: 19, scope: !236)
!236 = distinct !DILexicalBlock(scope: !179, file: !1, line: 229, column: 7)
!237 = !DILocation(line: 229, column: 7, scope: !236)
!238 = !DILocation(line: 229, column: 39, scope: !236)
!239 = !DILocation(line: 229, column: 7, scope: !179)
!240 = !DILocation(line: 233, column: 3, scope: !241)
!241 = distinct !DILexicalBlock(scope: !236, file: !1, line: 230, column: 3)
!242 = !DILocation(line: 241, column: 4, scope: !243)
!243 = distinct !DILexicalBlock(scope: !236, file: !1, line: 235, column: 3)
!244 = distinct !{!244, !175, !245, !98}
!245 = !DILocation(line: 303, column: 2, scope: !9)
!246 = !DILocation(line: 247, column: 24, scope: !247)
!247 = distinct !DILexicalBlock(scope: !179, file: !1, line: 247, column: 7)
!248 = !DILocation(line: 247, column: 7, scope: !179)
!249 = !DILocation(line: 251, column: 3, scope: !250)
!250 = distinct !DILexicalBlock(scope: !247, file: !1, line: 248, column: 3)
!251 = !DILocation(line: 252, column: 13, scope: !252)
!252 = distinct !DILexicalBlock(scope: !179, file: !1, line: 252, column: 7)
!253 = !DILocation(line: 252, column: 7, scope: !179)
!254 = !DILocation(line: 256, column: 3, scope: !255)
!255 = distinct !DILexicalBlock(scope: !252, file: !1, line: 253, column: 3)
!256 = !DILocation(line: 260, column: 18, scope: !179)
!257 = !DILocalVariable(name: "zRange", scope: !9, file: !1, line: 130, type: !107)
!258 = !DILocation(line: 262, column: 24, scope: !259)
!259 = distinct !DILexicalBlock(scope: !179, file: !1, line: 262, column: 7)
!260 = !DILocation(line: 262, column: 29, scope: !259)
!261 = !DILocation(line: 262, column: 49, scope: !259)
!262 = !DILocation(line: 262, column: 7, scope: !179)
!263 = !DILocation(line: 264, column: 25, scope: !264)
!264 = distinct !DILexicalBlock(scope: !259, file: !1, line: 263, column: 3)
!265 = !DILocation(line: 264, column: 34, scope: !264)
!266 = !DILocalVariable(name: "temp_z", scope: !264, file: !1, line: 270, type: !107)
!267 = !DILocation(line: 0, scope: !264)
!268 = !DILocation(line: 275, column: 3, scope: !264)
!269 = !DILocation(line: 280, column: 24, scope: !270)
!270 = distinct !DILexicalBlock(scope: !179, file: !1, line: 280, column: 7)
!271 = !DILocation(line: 281, column: 8, scope: !270)
!272 = !DILocation(line: 281, column: 23, scope: !270)
!273 = !DILocation(line: 282, column: 4, scope: !270)
!274 = !DILocation(line: 282, column: 17, scope: !270)
!275 = !DILocation(line: 282, column: 7, scope: !270)
!276 = !DILocation(line: 282, column: 37, scope: !270)
!277 = !DILocation(line: 283, column: 4, scope: !270)
!278 = !DILocation(line: 283, column: 14, scope: !270)
!279 = !DILocation(line: 283, column: 18, scope: !270)
!280 = !DILocation(line: 283, column: 28, scope: !270)
!281 = !DILocation(line: 280, column: 7, scope: !179)
!282 = !DILocation(line: 285, column: 9, scope: !283)
!283 = distinct !DILexicalBlock(scope: !270, file: !1, line: 284, column: 3)
!284 = !DILocation(line: 289, column: 3, scope: !283)
!285 = !DILocation(line: 291, column: 17, scope: !286)
!286 = distinct !DILexicalBlock(scope: !179, file: !1, line: 291, column: 7)
!287 = !DILocation(line: 291, column: 37, scope: !286)
!288 = !DILocation(line: 291, column: 41, scope: !286)
!289 = !DILocation(line: 291, column: 50, scope: !286)
!290 = !DILocation(line: 291, column: 7, scope: !179)
!291 = !DILocation(line: 298, column: 11, scope: !292)
!292 = distinct !DILexicalBlock(scope: !286, file: !1, line: 292, column: 3)
!293 = !DILocation(line: 300, column: 3, scope: !292)
!294 = !DILocation(line: 302, column: 19, scope: !179)
!295 = !DILocation(line: 305, column: 2, scope: !9)
!296 = !DILocation(line: 306, column: 20, scope: !9)
!297 = !DILocation(line: 306, column: 17, scope: !9)
!298 = !DILocation(line: 306, column: 41, scope: !9)
!299 = !DILocation(line: 306, column: 38, scope: !9)
!300 = !DILocation(line: 306, column: 36, scope: !9)
!301 = !DILocation(line: 306, column: 27, scope: !9)
!302 = !DILocalVariable(name: "t2", scope: !9, file: !1, line: 306, type: !174)
!303 = !DILocation(line: 308, column: 21, scope: !9)
!304 = !DILocation(line: 308, column: 2, scope: !9)
!305 = !DILocation(line: 312, column: 2, scope: !9)
!306 = !DILocation(line: 314, column: 2, scope: !9)
