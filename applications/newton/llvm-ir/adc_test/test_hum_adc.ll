; ModuleID = 'test_hum_adc.c'
source_filename = "test_hum_adc.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.bme680_dev = type { i8, i8, i32, i8, i8, %struct.bme680_calib_data, %struct.bme680_tph_sett, %struct.bme680_gas_sett, i8, i8, i8, i8 (i8, i8, i8*, i16)*, i8 (i8, i8, i8*, i16)*, void (i32)*, i8 }
%struct.bme680_calib_data = type { i32, i16, i8, i32, i8, i32, i8, i8, i16, i8, i16, i16, i8, i16, i16, i8, i16, i16, i8, i8, i16, i16, i8, i32, i8, i8, i8 }
%struct.bme680_tph_sett = type { i8, i8, i8, i8 }
%struct.bme680_gas_sett = type { i8, i8, i8, i16, i16 }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @user_delay_ms(i32 %0) #0 !dbg !22 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  call void @llvm.dbg.declare(metadata i32* %2, metadata !26, metadata !DIExpression()), !dbg !27
  ret void, !dbg !28
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local signext i8 @user_i2c_read(i8 zeroext %0, i8 zeroext %1, i8* %2, i16 zeroext %3) #0 !dbg !29 {
  %5 = alloca i8, align 1
  %6 = alloca i8, align 1
  %7 = alloca i8*, align 8
  %8 = alloca i16, align 2
  %9 = alloca i8, align 1
  store i8 %0, i8* %5, align 1
  call void @llvm.dbg.declare(metadata i8* %5, metadata !42, metadata !DIExpression()), !dbg !43
  store i8 %1, i8* %6, align 1
  call void @llvm.dbg.declare(metadata i8* %6, metadata !44, metadata !DIExpression()), !dbg !45
  store i8* %2, i8** %7, align 8
  call void @llvm.dbg.declare(metadata i8** %7, metadata !46, metadata !DIExpression()), !dbg !47
  store i16 %3, i16* %8, align 2
  call void @llvm.dbg.declare(metadata i16* %8, metadata !48, metadata !DIExpression()), !dbg !49
  call void @llvm.dbg.declare(metadata i8* %9, metadata !50, metadata !DIExpression()), !dbg !51
  store i8 0, i8* %9, align 1, !dbg !51
  %10 = load i8, i8* %9, align 1, !dbg !52
  ret i8 %10, !dbg !53
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local signext i8 @user_i2c_write(i8 zeroext %0, i8 zeroext %1, i8* %2, i16 zeroext %3) #0 !dbg !54 {
  %5 = alloca i8, align 1
  %6 = alloca i8, align 1
  %7 = alloca i8*, align 8
  %8 = alloca i16, align 2
  %9 = alloca i8, align 1
  store i8 %0, i8* %5, align 1
  call void @llvm.dbg.declare(metadata i8* %5, metadata !55, metadata !DIExpression()), !dbg !56
  store i8 %1, i8* %6, align 1
  call void @llvm.dbg.declare(metadata i8* %6, metadata !57, metadata !DIExpression()), !dbg !58
  store i8* %2, i8** %7, align 8
  call void @llvm.dbg.declare(metadata i8** %7, metadata !59, metadata !DIExpression()), !dbg !60
  store i16 %3, i16* %8, align 2
  call void @llvm.dbg.declare(metadata i16* %8, metadata !61, metadata !DIExpression()), !dbg !62
  call void @llvm.dbg.declare(metadata i8* %9, metadata !63, metadata !DIExpression()), !dbg !64
  store i8 0, i8* %9, align 1, !dbg !64
  %10 = load i8, i8* %9, align 1, !dbg !65
  ret i8 %10, !dbg !66
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !67 {
  %1 = alloca %struct.bme680_dev, align 8
  %2 = alloca i8, align 1
  %3 = alloca i32, align 4
  call void @llvm.dbg.declare(metadata %struct.bme680_dev* %1, metadata !70, metadata !DIExpression()), !dbg !146
  %4 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %1, i32 0, i32 1, !dbg !147
  store i8 118, i8* %4, align 1, !dbg !148
  %5 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %1, i32 0, i32 2, !dbg !149
  store i32 1, i32* %5, align 4, !dbg !150
  %6 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %1, i32 0, i32 11, !dbg !151
  store i8 (i8, i8, i8*, i16)* @user_i2c_read, i8 (i8, i8, i8*, i16)** %6, align 8, !dbg !152
  %7 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %1, i32 0, i32 12, !dbg !153
  store i8 (i8, i8, i8*, i16)* @user_i2c_write, i8 (i8, i8, i8*, i16)** %7, align 8, !dbg !154
  %8 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %1, i32 0, i32 13, !dbg !155
  store void (i32)* @user_delay_ms, void (i32)** %8, align 8, !dbg !156
  %9 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %1, i32 0, i32 4, !dbg !157
  store i8 25, i8* %9, align 1, !dbg !158
  call void @llvm.dbg.declare(metadata i8* %2, metadata !159, metadata !DIExpression()), !dbg !160
  store i8 0, i8* %2, align 1, !dbg !160
  %10 = call signext i8 @bme680_init(%struct.bme680_dev* %1), !dbg !161
  store i8 %10, i8* %2, align 1, !dbg !162
  call void @llvm.dbg.declare(metadata i32* %3, metadata !163, metadata !DIExpression()), !dbg !164
  store i32 10, i32* %3, align 4, !dbg !164
  %11 = load i32, i32* %3, align 4, !dbg !165
  %12 = call i32 @calc_humidity(i32 %11, %struct.bme680_dev* %1), !dbg !166
  ret i32 0, !dbg !167
}

declare dso_local signext i8 @bme680_init(%struct.bme680_dev*) #2

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @calc_humidity(i32 %0, %struct.bme680_dev* %1) #0 !dbg !168 {
  %3 = alloca i32, align 4
  %4 = alloca %struct.bme680_dev*, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca i32, align 4
  %13 = alloca i64, align 8
  %14 = alloca i64, align 8
  %15 = alloca i8, align 1
  store i32 %0, i32* %3, align 4
  call void @llvm.dbg.declare(metadata i32* %3, metadata !173, metadata !DIExpression()), !dbg !174
  store %struct.bme680_dev* %1, %struct.bme680_dev** %4, align 8
  call void @llvm.dbg.declare(metadata %struct.bme680_dev** %4, metadata !175, metadata !DIExpression()), !dbg !176
  call void @llvm.dbg.declare(metadata i32* %5, metadata !177, metadata !DIExpression()), !dbg !178
  call void @llvm.dbg.declare(metadata i32* %6, metadata !179, metadata !DIExpression()), !dbg !181
  call void @llvm.dbg.declare(metadata i32* %7, metadata !182, metadata !DIExpression()), !dbg !183
  call void @llvm.dbg.declare(metadata i32* %8, metadata !184, metadata !DIExpression()), !dbg !185
  call void @llvm.dbg.declare(metadata i32* %9, metadata !186, metadata !DIExpression()), !dbg !187
  call void @llvm.dbg.declare(metadata i32* %10, metadata !188, metadata !DIExpression()), !dbg !189
  call void @llvm.dbg.declare(metadata i32* %11, metadata !190, metadata !DIExpression()), !dbg !191
  call void @llvm.dbg.declare(metadata i32* %12, metadata !192, metadata !DIExpression()), !dbg !193
  call void @llvm.dbg.declare(metadata i64* %13, metadata !194, metadata !DIExpression()), !dbg !199
  store i64 1, i64* %13, align 8, !dbg !199
  call void @llvm.dbg.declare(metadata i64* %14, metadata !200, metadata !DIExpression()), !dbg !202
  store i64 1, i64* %14, align 8, !dbg !202
  call void @llvm.dbg.declare(metadata i8* %15, metadata !203, metadata !DIExpression()), !dbg !204
  store i8 1, i8* %15, align 1, !dbg !204
  %16 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !205
  %17 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %16, i32 0, i32 5, !dbg !206
  %18 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %17, i32 0, i32 23, !dbg !207
  %19 = load i32, i32* %18, align 4, !dbg !207
  %20 = mul nsw i32 %19, 5, !dbg !208
  %21 = sext i32 %20 to i64, !dbg !209
  %22 = load i64, i64* %13, align 8, !dbg !210
  %23 = mul nsw i64 %21, %22, !dbg !211
  %24 = add nsw i64 %23, 128, !dbg !212
  %25 = ashr i64 %24, 8, !dbg !213
  %26 = trunc i64 %25 to i32, !dbg !214
  store i32 %26, i32* %11, align 4, !dbg !215
  %27 = load i32, i32* %3, align 4, !dbg !216
  %28 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !217
  %29 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %28, i32 0, i32 5, !dbg !218
  %30 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %29, i32 0, i32 0, !dbg !219
  %31 = load i32, i32* %30, align 4, !dbg !219
  %32 = mul nsw i32 %31, 16, !dbg !220
  %33 = sub nsw i32 %27, %32, !dbg !221
  %34 = load i32, i32* %11, align 4, !dbg !222
  %35 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !223
  %36 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %35, i32 0, i32 5, !dbg !224
  %37 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %36, i32 0, i32 2, !dbg !225
  %38 = load i8, i8* %37, align 2, !dbg !225
  %39 = sext i8 %38 to i32, !dbg !226
  %40 = mul nsw i32 %34, %39, !dbg !227
  %41 = sdiv i32 %40, 100, !dbg !228
  %42 = ashr i32 %41, 1, !dbg !229
  %43 = sub nsw i32 %33, %42, !dbg !230
  store i32 %43, i32* %5, align 4, !dbg !231
  %44 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !232
  %45 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %44, i32 0, i32 5, !dbg !233
  %46 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %45, i32 0, i32 1, !dbg !234
  %47 = load i16, i16* %46, align 4, !dbg !234
  %48 = zext i16 %47 to i32, !dbg !235
  %49 = sext i32 %48 to i64, !dbg !235
  %50 = load i32, i32* %11, align 4, !dbg !236
  %51 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !237
  %52 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %51, i32 0, i32 5, !dbg !238
  %53 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %52, i32 0, i32 3, !dbg !239
  %54 = load i32, i32* %53, align 4, !dbg !239
  %55 = mul nsw i32 %50, %54, !dbg !240
  %56 = sdiv i32 %55, 100, !dbg !241
  %57 = load i32, i32* %11, align 4, !dbg !242
  %58 = load i32, i32* %11, align 4, !dbg !243
  %59 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !244
  %60 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %59, i32 0, i32 5, !dbg !245
  %61 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %60, i32 0, i32 4, !dbg !246
  %62 = load i8, i8* %61, align 4, !dbg !246
  %63 = sext i8 %62 to i32, !dbg !247
  %64 = mul nsw i32 %58, %63, !dbg !248
  %65 = sdiv i32 %64, 100, !dbg !249
  %66 = mul nsw i32 %57, %65, !dbg !250
  %67 = ashr i32 %66, 6, !dbg !251
  %68 = sdiv i32 %67, 100, !dbg !252
  %69 = add nsw i32 %56, %68, !dbg !253
  %70 = sext i32 %69 to i64, !dbg !254
  %71 = load i8, i8* %15, align 1, !dbg !255
  %72 = sext i8 %71 to i32, !dbg !255
  %73 = mul nsw i32 16384, %72, !dbg !256
  %74 = sext i32 %73 to i64, !dbg !257
  %75 = load i64, i64* %14, align 8, !dbg !258
  %76 = mul nsw i64 %74, %75, !dbg !259
  %77 = add nsw i64 %70, %76, !dbg !260
  %78 = mul nsw i64 %49, %77, !dbg !261
  %79 = ashr i64 %78, 10, !dbg !262
  %80 = trunc i64 %79 to i32, !dbg !263
  store i32 %80, i32* %6, align 4, !dbg !264
  %81 = load i32, i32* %5, align 4, !dbg !265
  %82 = load i32, i32* %6, align 4, !dbg !266
  %83 = mul nsw i32 %81, %82, !dbg !267
  store i32 %83, i32* %7, align 4, !dbg !268
  %84 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !269
  %85 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %84, i32 0, i32 5, !dbg !270
  %86 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %85, i32 0, i32 5, !dbg !271
  %87 = load i32, i32* %86, align 4, !dbg !271
  %88 = shl i32 %87, 7, !dbg !272
  store i32 %88, i32* %8, align 4, !dbg !273
  %89 = load i32, i32* %8, align 4, !dbg !274
  %90 = load i32, i32* %11, align 4, !dbg !275
  %91 = load %struct.bme680_dev*, %struct.bme680_dev** %4, align 8, !dbg !276
  %92 = getelementptr inbounds %struct.bme680_dev, %struct.bme680_dev* %91, i32 0, i32 5, !dbg !277
  %93 = getelementptr inbounds %struct.bme680_calib_data, %struct.bme680_calib_data* %92, i32 0, i32 6, !dbg !278
  %94 = load i8, i8* %93, align 4, !dbg !278
  %95 = sext i8 %94 to i32, !dbg !279
  %96 = mul nsw i32 %90, %95, !dbg !280
  %97 = sdiv i32 %96, 100, !dbg !281
  %98 = add nsw i32 %89, %97, !dbg !282
  %99 = ashr i32 %98, 4, !dbg !283
  store i32 %99, i32* %8, align 4, !dbg !284
  %100 = load i32, i32* %7, align 4, !dbg !285
  %101 = ashr i32 %100, 14, !dbg !286
  %102 = load i32, i32* %7, align 4, !dbg !287
  %103 = ashr i32 %102, 14, !dbg !288
  %104 = mul nsw i32 %101, %103, !dbg !289
  %105 = ashr i32 %104, 10, !dbg !290
  store i32 %105, i32* %9, align 4, !dbg !291
  %106 = load i32, i32* %8, align 4, !dbg !292
  %107 = load i32, i32* %9, align 4, !dbg !293
  %108 = mul nsw i32 %106, %107, !dbg !294
  %109 = ashr i32 %108, 1, !dbg !295
  store i32 %109, i32* %10, align 4, !dbg !296
  %110 = load i32, i32* %7, align 4, !dbg !297
  %111 = load i32, i32* %10, align 4, !dbg !298
  %112 = add nsw i32 %110, %111, !dbg !299
  %113 = ashr i32 %112, 10, !dbg !300
  %114 = mul nsw i32 %113, 1000, !dbg !301
  %115 = ashr i32 %114, 12, !dbg !302
  store i32 %115, i32* %12, align 4, !dbg !303
  %116 = load i32, i32* %12, align 4, !dbg !304
  %117 = icmp sgt i32 %116, 100000, !dbg !306
  br i1 %117, label %118, label %119, !dbg !307

118:                                              ; preds = %2
  store i32 100000, i32* %12, align 4, !dbg !308
  br label %124, !dbg !309

119:                                              ; preds = %2
  %120 = load i32, i32* %12, align 4, !dbg !310
  %121 = icmp slt i32 %120, 0, !dbg !312
  br i1 %121, label %122, label %123, !dbg !313

122:                                              ; preds = %119
  store i32 0, i32* %12, align 4, !dbg !314
  br label %123, !dbg !315

123:                                              ; preds = %122, %119
  br label %124

124:                                              ; preds = %123, %118
  %125 = load i32, i32* %12, align 4, !dbg !316
  ret i32 %125, !dbg !317
}

attributes #0 = { noinline nounwind optnone uwtable "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!18, !19, !20}
!llvm.ident = !{!21}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, retainedTypes: !9, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "test_hum_adc.c", directory: "/home/blackgeorge/Noisy-lang-compiler/applications/newton/llvm-ir/adc_test")
!2 = !{!3}
!3 = !DICompositeType(tag: DW_TAG_enumeration_type, name: "bme680_intf", file: !4, line: 359, baseType: !5, size: 32, elements: !6)
!4 = !DIFile(filename: "./bme680_defs.h", directory: "/home/blackgeorge/Noisy-lang-compiler/applications/newton/llvm-ir/adc_test")
!5 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!6 = !{!7, !8}
!7 = !DIEnumerator(name: "BME680_SPI_INTF", value: 0, isUnsigned: true)
!8 = !DIEnumerator(name: "BME680_I2C_INTF", value: 1, isUnsigned: true)
!9 = !{!10, !15}
!10 = !DIDerivedType(tag: DW_TAG_typedef, name: "int32_t", file: !11, line: 26, baseType: !12)
!11 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdint-intn.h", directory: "")
!12 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int32_t", file: !13, line: 41, baseType: !14)
!13 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/types.h", directory: "")
!14 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!15 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint32_t", file: !16, line: 26, baseType: !17)
!16 = !DIFile(filename: "/usr/include/x86_64-linux-gnu/bits/stdint-uintn.h", directory: "")
!17 = !DIDerivedType(tag: DW_TAG_typedef, name: "__uint32_t", file: !13, line: 42, baseType: !5)
!18 = !{i32 7, !"Dwarf Version", i32 4}
!19 = !{i32 2, !"Debug Info Version", i32 3}
!20 = !{i32 1, !"wchar_size", i32 4}
!21 = !{!"Ubuntu clang version 12.0.1-++20211102090516+fed41342a82f-1~exp1~20211102211019.11"}
!22 = distinct !DISubprogram(name: "user_delay_ms", scope: !1, file: !1, line: 5, type: !23, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !25)
!23 = !DISubroutineType(types: !24)
!24 = !{null, !15}
!25 = !{}
!26 = !DILocalVariable(name: "period", arg: 1, scope: !22, file: !1, line: 5, type: !15)
!27 = !DILocation(line: 5, column: 29, scope: !22)
!28 = !DILocation(line: 11, column: 1, scope: !22)
!29 = distinct !DISubprogram(name: "user_i2c_read", scope: !1, file: !1, line: 13, type: !30, scopeLine: 14, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !25)
!30 = !DISubroutineType(types: !31)
!31 = !{!32, !35, !35, !38, !39}
!32 = !DIDerivedType(tag: DW_TAG_typedef, name: "int8_t", file: !11, line: 24, baseType: !33)
!33 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int8_t", file: !13, line: 37, baseType: !34)
!34 = !DIBasicType(name: "signed char", size: 8, encoding: DW_ATE_signed_char)
!35 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint8_t", file: !16, line: 24, baseType: !36)
!36 = !DIDerivedType(tag: DW_TAG_typedef, name: "__uint8_t", file: !13, line: 38, baseType: !37)
!37 = !DIBasicType(name: "unsigned char", size: 8, encoding: DW_ATE_unsigned_char)
!38 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !35, size: 64)
!39 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint16_t", file: !16, line: 25, baseType: !40)
!40 = !DIDerivedType(tag: DW_TAG_typedef, name: "__uint16_t", file: !13, line: 40, baseType: !41)
!41 = !DIBasicType(name: "unsigned short", size: 16, encoding: DW_ATE_unsigned)
!42 = !DILocalVariable(name: "dev_id", arg: 1, scope: !29, file: !1, line: 13, type: !35)
!43 = !DILocation(line: 13, column: 30, scope: !29)
!44 = !DILocalVariable(name: "reg_addr", arg: 2, scope: !29, file: !1, line: 13, type: !35)
!45 = !DILocation(line: 13, column: 46, scope: !29)
!46 = !DILocalVariable(name: "reg_data", arg: 3, scope: !29, file: !1, line: 13, type: !38)
!47 = !DILocation(line: 13, column: 65, scope: !29)
!48 = !DILocalVariable(name: "len", arg: 4, scope: !29, file: !1, line: 13, type: !39)
!49 = !DILocation(line: 13, column: 84, scope: !29)
!50 = !DILocalVariable(name: "rslt", scope: !29, file: !1, line: 15, type: !32)
!51 = !DILocation(line: 15, column: 12, scope: !29)
!52 = !DILocation(line: 37, column: 12, scope: !29)
!53 = !DILocation(line: 37, column: 5, scope: !29)
!54 = distinct !DISubprogram(name: "user_i2c_write", scope: !1, file: !1, line: 41, type: !30, scopeLine: 42, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !25)
!55 = !DILocalVariable(name: "dev_id", arg: 1, scope: !54, file: !1, line: 41, type: !35)
!56 = !DILocation(line: 41, column: 31, scope: !54)
!57 = !DILocalVariable(name: "reg_addr", arg: 2, scope: !54, file: !1, line: 41, type: !35)
!58 = !DILocation(line: 41, column: 47, scope: !54)
!59 = !DILocalVariable(name: "reg_data", arg: 3, scope: !54, file: !1, line: 41, type: !38)
!60 = !DILocation(line: 41, column: 66, scope: !54)
!61 = !DILocalVariable(name: "len", arg: 4, scope: !54, file: !1, line: 41, type: !39)
!62 = !DILocation(line: 41, column: 85, scope: !54)
!63 = !DILocalVariable(name: "rslt", scope: !54, file: !1, line: 43, type: !32)
!64 = !DILocation(line: 43, column: 12, scope: !54)
!65 = !DILocation(line: 63, column: 12, scope: !54)
!66 = !DILocation(line: 63, column: 5, scope: !54)
!67 = distinct !DISubprogram(name: "main", scope: !1, file: !1, line: 105, type: !68, scopeLine: 105, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !25)
!68 = !DISubroutineType(types: !69)
!69 = !{!14}
!70 = !DILocalVariable(name: "gas_sensor", scope: !67, file: !1, line: 106, type: !71)
!71 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "bme680_dev", file: !4, line: 502, size: 960, elements: !72)
!72 = !{!73, !74, !75, !76, !77, !78, !120, !127, !135, !136, !137, !138, !141, !142, !145}
!73 = !DIDerivedType(tag: DW_TAG_member, name: "chip_id", scope: !71, file: !4, line: 504, baseType: !35, size: 8)
!74 = !DIDerivedType(tag: DW_TAG_member, name: "dev_id", scope: !71, file: !4, line: 506, baseType: !35, size: 8, offset: 8)
!75 = !DIDerivedType(tag: DW_TAG_member, name: "intf", scope: !71, file: !4, line: 508, baseType: !3, size: 32, offset: 32)
!76 = !DIDerivedType(tag: DW_TAG_member, name: "mem_page", scope: !71, file: !4, line: 510, baseType: !35, size: 8, offset: 64)
!77 = !DIDerivedType(tag: DW_TAG_member, name: "amb_temp", scope: !71, file: !4, line: 512, baseType: !32, size: 8, offset: 72)
!78 = !DIDerivedType(tag: DW_TAG_member, name: "calib", scope: !71, file: !4, line: 514, baseType: !79, size: 480, offset: 96)
!79 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "bme680_calib_data", file: !4, line: 404, size: 480, elements: !80)
!80 = !{!81, !84, !86, !88, !90, !92, !94, !96, !97, !101, !102, !103, !104, !105, !106, !107, !108, !109, !110, !111, !112, !113, !114, !115, !117, !118, !119}
!81 = !DIDerivedType(tag: DW_TAG_member, name: "par_h1", scope: !79, file: !4, line: 406, baseType: !82, size: 32)
!82 = !DIDerivedType(tag: DW_TAG_typedef, name: "voltage", file: !83, line: 15, baseType: !10)
!83 = !DIFile(filename: "./signalTypedef.h", directory: "/home/blackgeorge/Noisy-lang-compiler/applications/newton/llvm-ir/adc_test")
!84 = !DIDerivedType(tag: DW_TAG_member, name: "par_h2", scope: !79, file: !4, line: 408, baseType: !85, size: 16, offset: 32)
!85 = !DIDerivedType(tag: DW_TAG_typedef, name: "relativeHumidityPerVoltageTemperatureSquaredDimension", file: !83, line: 10, baseType: !39)
!86 = !DIDerivedType(tag: DW_TAG_member, name: "par_h3", scope: !79, file: !4, line: 410, baseType: !87, size: 8, offset: 48)
!87 = !DIDerivedType(tag: DW_TAG_typedef, name: "VoltagePerTemperatureDimension", file: !83, line: 6, baseType: !32)
!88 = !DIDerivedType(tag: DW_TAG_member, name: "par_h4", scope: !79, file: !4, line: 412, baseType: !89, size: 32, offset: 64)
!89 = !DIDerivedType(tag: DW_TAG_typedef, name: "temperature", file: !83, line: 14, baseType: !10)
!90 = !DIDerivedType(tag: DW_TAG_member, name: "par_h5", scope: !79, file: !4, line: 414, baseType: !91, size: 8, offset: 96)
!91 = !DIDerivedType(tag: DW_TAG_typedef, name: "unitless", file: !83, line: 7, baseType: !32)
!92 = !DIDerivedType(tag: DW_TAG_member, name: "par_h6", scope: !79, file: !4, line: 416, baseType: !93, size: 32, offset: 128)
!93 = !DIDerivedType(tag: DW_TAG_typedef, name: "relativeHumidity", file: !83, line: 12, baseType: !10)
!94 = !DIDerivedType(tag: DW_TAG_member, name: "par_h7", scope: !79, file: !4, line: 418, baseType: !95, size: 8, offset: 160)
!95 = !DIDerivedType(tag: DW_TAG_typedef, name: "relativeHumidityPerTemperature", file: !83, line: 8, baseType: !32)
!96 = !DIDerivedType(tag: DW_TAG_member, name: "par_gh1", scope: !79, file: !4, line: 420, baseType: !32, size: 8, offset: 168)
!97 = !DIDerivedType(tag: DW_TAG_member, name: "par_gh2", scope: !79, file: !4, line: 422, baseType: !98, size: 16, offset: 176)
!98 = !DIDerivedType(tag: DW_TAG_typedef, name: "int16_t", file: !11, line: 25, baseType: !99)
!99 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int16_t", file: !13, line: 39, baseType: !100)
!100 = !DIBasicType(name: "short", size: 16, encoding: DW_ATE_signed)
!101 = !DIDerivedType(tag: DW_TAG_member, name: "par_gh3", scope: !79, file: !4, line: 424, baseType: !32, size: 8, offset: 192)
!102 = !DIDerivedType(tag: DW_TAG_member, name: "par_t1", scope: !79, file: !4, line: 426, baseType: !39, size: 16, offset: 208)
!103 = !DIDerivedType(tag: DW_TAG_member, name: "par_t2", scope: !79, file: !4, line: 428, baseType: !98, size: 16, offset: 224)
!104 = !DIDerivedType(tag: DW_TAG_member, name: "par_t3", scope: !79, file: !4, line: 430, baseType: !32, size: 8, offset: 240)
!105 = !DIDerivedType(tag: DW_TAG_member, name: "par_p1", scope: !79, file: !4, line: 432, baseType: !39, size: 16, offset: 256)
!106 = !DIDerivedType(tag: DW_TAG_member, name: "par_p2", scope: !79, file: !4, line: 434, baseType: !98, size: 16, offset: 272)
!107 = !DIDerivedType(tag: DW_TAG_member, name: "par_p3", scope: !79, file: !4, line: 436, baseType: !32, size: 8, offset: 288)
!108 = !DIDerivedType(tag: DW_TAG_member, name: "par_p4", scope: !79, file: !4, line: 438, baseType: !98, size: 16, offset: 304)
!109 = !DIDerivedType(tag: DW_TAG_member, name: "par_p5", scope: !79, file: !4, line: 440, baseType: !98, size: 16, offset: 320)
!110 = !DIDerivedType(tag: DW_TAG_member, name: "par_p6", scope: !79, file: !4, line: 442, baseType: !32, size: 8, offset: 336)
!111 = !DIDerivedType(tag: DW_TAG_member, name: "par_p7", scope: !79, file: !4, line: 444, baseType: !32, size: 8, offset: 344)
!112 = !DIDerivedType(tag: DW_TAG_member, name: "par_p8", scope: !79, file: !4, line: 446, baseType: !98, size: 16, offset: 352)
!113 = !DIDerivedType(tag: DW_TAG_member, name: "par_p9", scope: !79, file: !4, line: 448, baseType: !98, size: 16, offset: 368)
!114 = !DIDerivedType(tag: DW_TAG_member, name: "par_p10", scope: !79, file: !4, line: 450, baseType: !35, size: 8, offset: 384)
!115 = !DIDerivedType(tag: DW_TAG_member, name: "t_fine", scope: !79, file: !4, line: 454, baseType: !116, size: 32, offset: 416)
!116 = !DIDerivedType(tag: DW_TAG_typedef, name: "VoltageSquaredDimension", file: !83, line: 16, baseType: !10)
!117 = !DIDerivedType(tag: DW_TAG_member, name: "res_heat_range", scope: !79, file: !4, line: 460, baseType: !35, size: 8, offset: 448)
!118 = !DIDerivedType(tag: DW_TAG_member, name: "res_heat_val", scope: !79, file: !4, line: 462, baseType: !32, size: 8, offset: 456)
!119 = !DIDerivedType(tag: DW_TAG_member, name: "range_sw_err", scope: !79, file: !4, line: 464, baseType: !32, size: 8, offset: 464)
!120 = !DIDerivedType(tag: DW_TAG_member, name: "tph_sett", scope: !71, file: !4, line: 516, baseType: !121, size: 32, offset: 576)
!121 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "bme680_tph_sett", file: !4, line: 471, size: 32, elements: !122)
!122 = !{!123, !124, !125, !126}
!123 = !DIDerivedType(tag: DW_TAG_member, name: "os_hum", scope: !121, file: !4, line: 473, baseType: !35, size: 8)
!124 = !DIDerivedType(tag: DW_TAG_member, name: "os_temp", scope: !121, file: !4, line: 475, baseType: !35, size: 8, offset: 8)
!125 = !DIDerivedType(tag: DW_TAG_member, name: "os_pres", scope: !121, file: !4, line: 477, baseType: !35, size: 8, offset: 16)
!126 = !DIDerivedType(tag: DW_TAG_member, name: "filter", scope: !121, file: !4, line: 479, baseType: !35, size: 8, offset: 24)
!127 = !DIDerivedType(tag: DW_TAG_member, name: "gas_sett", scope: !71, file: !4, line: 518, baseType: !128, size: 64, offset: 608)
!128 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "bme680_gas_sett", file: !4, line: 486, size: 64, elements: !129)
!129 = !{!130, !131, !132, !133, !134}
!130 = !DIDerivedType(tag: DW_TAG_member, name: "nb_conv", scope: !128, file: !4, line: 488, baseType: !35, size: 8)
!131 = !DIDerivedType(tag: DW_TAG_member, name: "heatr_ctrl", scope: !128, file: !4, line: 490, baseType: !35, size: 8, offset: 8)
!132 = !DIDerivedType(tag: DW_TAG_member, name: "run_gas", scope: !128, file: !4, line: 492, baseType: !35, size: 8, offset: 16)
!133 = !DIDerivedType(tag: DW_TAG_member, name: "heatr_temp", scope: !128, file: !4, line: 494, baseType: !39, size: 16, offset: 32)
!134 = !DIDerivedType(tag: DW_TAG_member, name: "heatr_dur", scope: !128, file: !4, line: 496, baseType: !39, size: 16, offset: 48)
!135 = !DIDerivedType(tag: DW_TAG_member, name: "power_mode", scope: !71, file: !4, line: 520, baseType: !35, size: 8, offset: 672)
!136 = !DIDerivedType(tag: DW_TAG_member, name: "new_fields", scope: !71, file: !4, line: 522, baseType: !35, size: 8, offset: 680)
!137 = !DIDerivedType(tag: DW_TAG_member, name: "info_msg", scope: !71, file: !4, line: 524, baseType: !35, size: 8, offset: 688)
!138 = !DIDerivedType(tag: DW_TAG_member, name: "read", scope: !71, file: !4, line: 526, baseType: !139, size: 64, offset: 704)
!139 = !DIDerivedType(tag: DW_TAG_typedef, name: "bme680_com_fptr_t", file: !4, line: 348, baseType: !140)
!140 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !30, size: 64)
!141 = !DIDerivedType(tag: DW_TAG_member, name: "write", scope: !71, file: !4, line: 528, baseType: !139, size: 64, offset: 768)
!142 = !DIDerivedType(tag: DW_TAG_member, name: "delay_ms", scope: !71, file: !4, line: 530, baseType: !143, size: 64, offset: 832)
!143 = !DIDerivedType(tag: DW_TAG_typedef, name: "bme680_delay_fptr_t", file: !4, line: 354, baseType: !144)
!144 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !23, size: 64)
!145 = !DIDerivedType(tag: DW_TAG_member, name: "com_rslt", scope: !71, file: !4, line: 532, baseType: !32, size: 8, offset: 896)
!146 = !DILocation(line: 106, column: 20, scope: !67)
!147 = !DILocation(line: 108, column: 13, scope: !67)
!148 = !DILocation(line: 108, column: 20, scope: !67)
!149 = !DILocation(line: 109, column: 13, scope: !67)
!150 = !DILocation(line: 109, column: 18, scope: !67)
!151 = !DILocation(line: 110, column: 13, scope: !67)
!152 = !DILocation(line: 110, column: 18, scope: !67)
!153 = !DILocation(line: 111, column: 13, scope: !67)
!154 = !DILocation(line: 111, column: 19, scope: !67)
!155 = !DILocation(line: 112, column: 13, scope: !67)
!156 = !DILocation(line: 112, column: 22, scope: !67)
!157 = !DILocation(line: 116, column: 13, scope: !67)
!158 = !DILocation(line: 116, column: 22, scope: !67)
!159 = !DILocalVariable(name: "rslt", scope: !67, file: !1, line: 118, type: !32)
!160 = !DILocation(line: 118, column: 9, scope: !67)
!161 = !DILocation(line: 119, column: 9, scope: !67)
!162 = !DILocation(line: 119, column: 7, scope: !67)
!163 = !DILocalVariable(name: "temp_adc", scope: !67, file: !1, line: 121, type: !15)
!164 = !DILocation(line: 121, column: 11, scope: !67)
!165 = !DILocation(line: 122, column: 16, scope: !67)
!166 = !DILocation(line: 122, column: 2, scope: !67)
!167 = !DILocation(line: 124, column: 1, scope: !67)
!168 = distinct !DISubprogram(name: "calc_humidity", scope: !1, file: !1, line: 67, type: !169, scopeLine: 68, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !0, retainedNodes: !25)
!169 = !DISubroutineType(types: !170)
!170 = !{!15, !82, !171}
!171 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !172, size: 64)
!172 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !71)
!173 = !DILocalVariable(name: "hum_adc", arg: 1, scope: !168, file: !1, line: 67, type: !82)
!174 = !DILocation(line: 67, column: 39, scope: !168)
!175 = !DILocalVariable(name: "dev", arg: 2, scope: !168, file: !1, line: 67, type: !171)
!176 = !DILocation(line: 67, column: 73, scope: !168)
!177 = !DILocalVariable(name: "var1", scope: !168, file: !1, line: 69, type: !82)
!178 = !DILocation(line: 69, column: 10, scope: !168)
!179 = !DILocalVariable(name: "var2", scope: !168, file: !1, line: 70, type: !180)
!180 = !DIDerivedType(tag: DW_TAG_typedef, name: "relativeHumidityPerVoltage", file: !83, line: 13, baseType: !10)
!181 = !DILocation(line: 70, column: 29, scope: !168)
!182 = !DILocalVariable(name: "var3", scope: !168, file: !1, line: 71, type: !93)
!183 = !DILocation(line: 71, column: 19, scope: !168)
!184 = !DILocalVariable(name: "var4", scope: !168, file: !1, line: 72, type: !93)
!185 = !DILocation(line: 72, column: 19, scope: !168)
!186 = !DILocalVariable(name: "var5", scope: !168, file: !1, line: 73, type: !93)
!187 = !DILocation(line: 73, column: 19, scope: !168)
!188 = !DILocalVariable(name: "var6", scope: !168, file: !1, line: 74, type: !93)
!189 = !DILocation(line: 74, column: 19, scope: !168)
!190 = !DILocalVariable(name: "temp_scaled", scope: !168, file: !1, line: 75, type: !89)
!191 = !DILocation(line: 75, column: 14, scope: !168)
!192 = !DILocalVariable(name: "calc_hum", scope: !168, file: !1, line: 76, type: !93)
!193 = !DILocation(line: 76, column: 19, scope: !168)
!194 = !DILocalVariable(name: "unitHavingConstant1", scope: !168, file: !1, line: 78, type: !195)
!195 = !DIDerivedType(tag: DW_TAG_typedef, name: "temperaturePerVoltageSquaredDimension", file: !83, line: 18, baseType: !196)
!196 = !DIDerivedType(tag: DW_TAG_typedef, name: "int64_t", file: !11, line: 27, baseType: !197)
!197 = !DIDerivedType(tag: DW_TAG_typedef, name: "__int64_t", file: !13, line: 44, baseType: !198)
!198 = !DIBasicType(name: "long int", size: 64, encoding: DW_ATE_signed)
!199 = !DILocation(line: 78, column: 41, scope: !168)
!200 = !DILocalVariable(name: "unitHavingConstant2", scope: !168, file: !1, line: 79, type: !201)
!201 = !DIDerivedType(tag: DW_TAG_typedef, name: "temperatureSquaredDimension", file: !83, line: 19, baseType: !196)
!202 = !DILocation(line: 79, column: 31, scope: !168)
!203 = !DILocalVariable(name: "temp_u", scope: !168, file: !1, line: 80, type: !91)
!204 = !DILocation(line: 80, column: 14, scope: !168)
!205 = !DILocation(line: 82, column: 28, scope: !168)
!206 = !DILocation(line: 82, column: 33, scope: !168)
!207 = !DILocation(line: 82, column: 39, scope: !168)
!208 = !DILocation(line: 82, column: 46, scope: !168)
!209 = !DILocation(line: 82, column: 17, scope: !168)
!210 = !DILocation(line: 82, column: 51, scope: !168)
!211 = !DILocation(line: 82, column: 50, scope: !168)
!212 = !DILocation(line: 82, column: 71, scope: !168)
!213 = !DILocation(line: 82, column: 78, scope: !168)
!214 = !DILocation(line: 82, column: 16, scope: !168)
!215 = !DILocation(line: 82, column: 14, scope: !168)
!216 = !DILocation(line: 83, column: 20, scope: !168)
!217 = !DILocation(line: 83, column: 52, scope: !168)
!218 = !DILocation(line: 83, column: 57, scope: !168)
!219 = !DILocation(line: 83, column: 63, scope: !168)
!220 = !DILocation(line: 83, column: 70, scope: !168)
!221 = !DILocation(line: 83, column: 28, scope: !168)
!222 = !DILocation(line: 84, column: 8, scope: !168)
!223 = !DILocation(line: 84, column: 32, scope: !168)
!224 = !DILocation(line: 84, column: 37, scope: !168)
!225 = !DILocation(line: 84, column: 43, scope: !168)
!226 = !DILocation(line: 84, column: 22, scope: !168)
!227 = !DILocation(line: 84, column: 20, scope: !168)
!228 = !DILocation(line: 84, column: 51, scope: !168)
!229 = !DILocation(line: 84, column: 70, scope: !168)
!230 = !DILocation(line: 84, column: 3, scope: !168)
!231 = !DILocation(line: 83, column: 7, scope: !168)
!232 = !DILocation(line: 85, column: 20, scope: !168)
!233 = !DILocation(line: 85, column: 25, scope: !168)
!234 = !DILocation(line: 85, column: 31, scope: !168)
!235 = !DILocation(line: 85, column: 10, scope: !168)
!236 = !DILocation(line: 86, column: 8, scope: !168)
!237 = !DILocation(line: 86, column: 32, scope: !168)
!238 = !DILocation(line: 86, column: 37, scope: !168)
!239 = !DILocation(line: 86, column: 43, scope: !168)
!240 = !DILocation(line: 86, column: 20, scope: !168)
!241 = !DILocation(line: 86, column: 51, scope: !168)
!242 = !DILocation(line: 87, column: 9, scope: !168)
!243 = !DILocation(line: 87, column: 25, scope: !168)
!244 = !DILocation(line: 87, column: 49, scope: !168)
!245 = !DILocation(line: 87, column: 54, scope: !168)
!246 = !DILocation(line: 87, column: 60, scope: !168)
!247 = !DILocation(line: 87, column: 39, scope: !168)
!248 = !DILocation(line: 87, column: 37, scope: !168)
!249 = !DILocation(line: 87, column: 68, scope: !168)
!250 = !DILocation(line: 87, column: 21, scope: !168)
!251 = !DILocation(line: 87, column: 88, scope: !168)
!252 = !DILocation(line: 88, column: 5, scope: !168)
!253 = !DILocation(line: 87, column: 4, scope: !168)
!254 = !DILocation(line: 86, column: 6, scope: !168)
!255 = !DILocation(line: 88, column: 46, scope: !168)
!256 = !DILocation(line: 88, column: 45, scope: !168)
!257 = !DILocation(line: 88, column: 26, scope: !168)
!258 = !DILocation(line: 88, column: 53, scope: !168)
!259 = !DILocation(line: 88, column: 52, scope: !168)
!260 = !DILocation(line: 88, column: 24, scope: !168)
!261 = !DILocation(line: 86, column: 3, scope: !168)
!262 = !DILocation(line: 88, column: 75, scope: !168)
!263 = !DILocation(line: 85, column: 9, scope: !168)
!264 = !DILocation(line: 85, column: 7, scope: !168)
!265 = !DILocation(line: 89, column: 9, scope: !168)
!266 = !DILocation(line: 89, column: 16, scope: !168)
!267 = !DILocation(line: 89, column: 14, scope: !168)
!268 = !DILocation(line: 89, column: 7, scope: !168)
!269 = !DILocation(line: 90, column: 19, scope: !168)
!270 = !DILocation(line: 90, column: 24, scope: !168)
!271 = !DILocation(line: 90, column: 30, scope: !168)
!272 = !DILocation(line: 90, column: 37, scope: !168)
!273 = !DILocation(line: 90, column: 7, scope: !168)
!274 = !DILocation(line: 91, column: 11, scope: !168)
!275 = !DILocation(line: 91, column: 21, scope: !168)
!276 = !DILocation(line: 91, column: 45, scope: !168)
!277 = !DILocation(line: 91, column: 50, scope: !168)
!278 = !DILocation(line: 91, column: 56, scope: !168)
!279 = !DILocation(line: 91, column: 35, scope: !168)
!280 = !DILocation(line: 91, column: 33, scope: !168)
!281 = !DILocation(line: 91, column: 64, scope: !168)
!282 = !DILocation(line: 91, column: 17, scope: !168)
!283 = !DILocation(line: 91, column: 84, scope: !168)
!284 = !DILocation(line: 91, column: 7, scope: !168)
!285 = !DILocation(line: 92, column: 11, scope: !168)
!286 = !DILocation(line: 92, column: 16, scope: !168)
!287 = !DILocation(line: 92, column: 26, scope: !168)
!288 = !DILocation(line: 92, column: 31, scope: !168)
!289 = !DILocation(line: 92, column: 23, scope: !168)
!290 = !DILocation(line: 92, column: 39, scope: !168)
!291 = !DILocation(line: 92, column: 7, scope: !168)
!292 = !DILocation(line: 93, column: 10, scope: !168)
!293 = !DILocation(line: 93, column: 17, scope: !168)
!294 = !DILocation(line: 93, column: 15, scope: !168)
!295 = !DILocation(line: 93, column: 23, scope: !168)
!296 = !DILocation(line: 93, column: 7, scope: !168)
!297 = !DILocation(line: 94, column: 16, scope: !168)
!298 = !DILocation(line: 94, column: 23, scope: !168)
!299 = !DILocation(line: 94, column: 21, scope: !168)
!300 = !DILocation(line: 94, column: 29, scope: !168)
!301 = !DILocation(line: 94, column: 36, scope: !168)
!302 = !DILocation(line: 94, column: 56, scope: !168)
!303 = !DILocation(line: 94, column: 11, scope: !168)
!304 = !DILocation(line: 96, column: 6, scope: !305)
!305 = distinct !DILexicalBlock(scope: !168, file: !1, line: 96, column: 6)
!306 = !DILocation(line: 96, column: 15, scope: !305)
!307 = !DILocation(line: 96, column: 6, scope: !168)
!308 = !DILocation(line: 97, column: 12, scope: !305)
!309 = !DILocation(line: 97, column: 3, scope: !305)
!310 = !DILocation(line: 98, column: 11, scope: !311)
!311 = distinct !DILexicalBlock(scope: !305, file: !1, line: 98, column: 11)
!312 = !DILocation(line: 98, column: 20, scope: !311)
!313 = !DILocation(line: 98, column: 11, scope: !305)
!314 = !DILocation(line: 99, column: 12, scope: !311)
!315 = !DILocation(line: 99, column: 3, scope: !311)
!316 = !DILocation(line: 101, column: 20, scope: !168)
!317 = !DILocation(line: 101, column: 2, scope: !168)
