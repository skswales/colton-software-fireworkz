@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2014-2021 Stuart Swales

@rem Assumes N: is mapped to the root of your development tree i.e. the dir containing \fireworkz
@rem You can easily do this with a batch file dropped in your Startup folder
@rem     SUBST N: %USERPROFILE%\cs-dev
@rem or some appropriate equivalent for your dev environment.
@rem If not, you just have to navigate to this directory before executing the batch file
set EXAMPLES_SRC_DIR=N:\fireworkz\%FIREWORKZ_TBT%\examples\Apps\Document\FzExamples
@rem
set EXAMPLES_OUT_DIR=%1
@rem
mkdir %EXAMPLES_OUT_DIR%
@rem
@rem Copy the directory structure
xcopy /T %EXAMPLES_SRC_DIR% %EXAMPLES_OUT_DIR%
@rem
copy /A "%EXAMPLES_SRC_DIR%\NewFeature\AutoFormat,bdf"          %EXAMPLES_OUT_DIR%\NewFeature\AutoFormat.fwk
copy /A "%EXAMPLES_SRC_DIR%\NewFeature\ButtonBar,bdf"           %EXAMPLES_OUT_DIR%\NewFeature\ButtonBar.fwk
copy /A "%EXAMPLES_SRC_DIR%\NewFeature\InCellEdit,bdf"          %EXAMPLES_OUT_DIR%\NewFeature\InCellEdit.fwk
copy /A "%EXAMPLES_SRC_DIR%\NewFeature\NumStyles,bdf"           %EXAMPLES_OUT_DIR%\NewFeature\NumStyles.fwk
copy /A "%EXAMPLES_SRC_DIR%\NewFeature\Stripes,bdf"             %EXAMPLES_OUT_DIR%\NewFeature\Stripes.fwk
copy /A "%EXAMPLES_SRC_DIR%\NewFeature\ValueLists,bdf"          %EXAMPLES_OUT_DIR%\NewFeature\ValueLists.fwk
@rem
rename  "%EXAMPLES_OUT_DIR%\NewFeature"                         %EXAMPLES_OUT_DIR%\NewFeatures
@rem
copy /A "%EXAMPLES_SRC_DIR%\Tutorial\Accounts,bdf"              %EXAMPLES_OUT_DIR%\Tutorial\Accounts.fwk
copy /A "%EXAMPLES_SRC_DIR%\Tutorial\Exams,bdf"                 %EXAMPLES_OUT_DIR%\Tutorial\Exams.fwk
copy /B "%EXAMPLES_SRC_DIR%\Tutorial\firepic,ff9"               %EXAMPLES_OUT_DIR%\Tutorial\firepic.ff9
copy /A "%EXAMPLES_SRC_DIR%\Tutorial\Form,bdf"                  %EXAMPLES_OUT_DIR%\Tutorial\Form.fwk
@rem FPro copy /A "%EXAMPLES_SRC_DIR%\Tutorial\names,bdf"       %EXAMPLES_OUT_DIR%\Tutorial\names.fwk
@rem FPro copy /B "%EXAMPLES_SRC_DIR%\Tutorial\names_b,c27"     %EXAMPLES_OUT_DIR%\Tutorial\names_b.c27
copy /A "%EXAMPLES_SRC_DIR%\Tutorial\Newsletter,bdf"            %EXAMPLES_OUT_DIR%\Tutorial\Newsletter.fwk
copy /A "%EXAMPLES_SRC_DIR%\Tutorial\Report,bdf"                %EXAMPLES_OUT_DIR%\Tutorial\Report.fwk
copy /A "%EXAMPLES_SRC_DIR%\Tutorial\saleslet2,bdf"             %EXAMPLES_OUT_DIR%\Tutorial\saleslet2.fwk
copy /A "%EXAMPLES_SRC_DIR%\Tutorial\Theatre,bdf"               %EXAMPLES_OUT_DIR%\Tutorial\Theatre.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Chars,bdf"                 %EXAMPLES_OUT_DIR%\Examples\Chars.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Commands,bdf"              %EXAMPLES_OUT_DIR%\Examples\Commands.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Keystrokes,bdf"            %EXAMPLES_OUT_DIR%\Examples\Keystrokes.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Numforms,bdf"              %EXAMPLES_OUT_DIR%\Examples\Numforms.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\YearPlan20,bdf"            %EXAMPLES_OUT_DIR%\Examples\YearPlan20.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\Bar,bdf"            %EXAMPLES_OUT_DIR%\Examples\Charts\Bar.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\Bar_unempl,bdf"     %EXAMPLES_OUT_DIR%\Examples\Charts\Bar_unempl.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\HistExam,bdf"       %EXAMPLES_OUT_DIR%\Examples\Charts\HistExam.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\HistRead,bdf"       %EXAMPLES_OUT_DIR%\Examples\Charts\HistRead.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\Line,bdf"           %EXAMPLES_OUT_DIR%\Examples\Charts\Line.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\Penguin,bdf"        %EXAMPLES_OUT_DIR%\Examples\Charts\Penguin.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\Pie,bdf"            %EXAMPLES_OUT_DIR%\Examples\Charts\Pie.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Charts\XY,bdf"             %EXAMPLES_OUT_DIR%\Examples\Charts\XY.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\Custom,bdf"         %EXAMPLES_OUT_DIR%\Examples\Custom\Custom.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\c_Fri13,bdf"        %EXAMPLES_OUT_DIR%\Examples\Custom\c_Fri13.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\c_tangen,bdf"       %EXAMPLES_OUT_DIR%\Examples\Custom\c_tangen.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\c_total,bdf"        %EXAMPLES_OUT_DIR%\Examples\Custom\c_total.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\Fri13,bdf"          %EXAMPLES_OUT_DIR%\Examples\Custom\Fri13.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\Fri13Read,bdf"      %EXAMPLES_OUT_DIR%\Examples\Custom\Fri13Read.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\tangent,bdf"        %EXAMPLES_OUT_DIR%\Examples\Custom\tangent.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Custom\total,bdf"          %EXAMPLES_OUT_DIR%\Examples\Custom\total.fwk
@rem
rd /q                                                           %EXAMPLES_OUT_DIR%\Examples\DB_Lookup                            
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Draft\draft_db,c1d"        %EXAMPLES_OUT_DIR%\Examples\Draft\draft_db.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Draft\draft_rz,c1d"        %EXAMPLES_OUT_DIR%\Examples\Draft\draft_rz.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Draft\draft_wz,c1d"        %EXAMPLES_OUT_DIR%\Examples\Draft\draft_wz.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Draft\ReadMe,bdf"          %EXAMPLES_OUT_DIR%\Examples\Draft\ReadMe.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Draft\test,bdf"            %EXAMPLES_OUT_DIR%\Examples\Draft\test.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\complex,bdf"     %EXAMPLES_OUT_DIR%\Examples\Functions\complex.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\database,bdf"    %EXAMPLES_OUT_DIR%\Examples\Functions\database.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\date,bdf"        %EXAMPLES_OUT_DIR%\Examples\Functions\date.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\finance,bdf"     %EXAMPLES_OUT_DIR%\Examples\Functions\finance.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\lookup,bdf"      %EXAMPLES_OUT_DIR%\Examples\Functions\lookup.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\matrix,bdf"      %EXAMPLES_OUT_DIR%\Examples\Functions\matrix.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\misc,bdf"        %EXAMPLES_OUT_DIR%\Examples\Functions\misc.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\string,bdf"      %EXAMPLES_OUT_DIR%\Examples\Functions\string.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\trig,bdf"        %EXAMPLES_OUT_DIR%\Examples\Functions\trig.fwk

copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Maths\Maths,bdf"             %EXAMPLES_OUT_DIR%\Examples\Functions\Maths\Maths.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Maths\Quotient,bdf"          %EXAMPLES_OUT_DIR%\Examples\Functions\Maths\Quotient.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Maths\SeriesSum,bdf"         %EXAMPLES_OUT_DIR%\Examples\Functions\Maths\SeriesSum.fwk

copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Statistics\Linest,bdf"       %EXAMPLES_OUT_DIR%\Examples\Functions\Statistics\Linest.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Statistics\Linest_m,bdf"     %EXAMPLES_OUT_DIR%\Examples\Functions\Statistics\Linest_m.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Statistics\Listcount,bdf"    %EXAMPLES_OUT_DIR%\Examples\Functions\Statistics\Listcount.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Statistics\Statistic,bdf"    %EXAMPLES_OUT_DIR%\Examples\Functions\Statistics\Statistic.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Functions\Statistics\Spearman,bdf"     %EXAMPLES_OUT_DIR%\Examples\Functions\Statistics\Spearman.fwk

@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Labels\csv_file,dfe"       %EXAMPLES_OUT_DIR%\Examples\Labels\csv_file.csv
copy /A "%EXAMPLES_SRC_DIR%\Examples\Labels\L7161,bdf"          %EXAMPLES_OUT_DIR%\Examples\Labels\L7161.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Labels\Label3x8,bdf"       %EXAMPLES_OUT_DIR%\Examples\Labels\Label3x8.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Labels\ReadMe,bdf"         %EXAMPLES_OUT_DIR%\Examples\Labels\ReadMe.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\MineSweep\!PlayMe,bdf"     %EXAMPLES_OUT_DIR%\Examples\MineSweep\!PlayMe.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\MineSweep\Board20,bdf"     %EXAMPLES_OUT_DIR%\Examples\MineSweep\Board20.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\MineSweep\c_game,bdf"      %EXAMPLES_OUT_DIR%\Examples\MineSweep\c_game.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\MineSweep\ReadMe,bdf"      %EXAMPLES_OUT_DIR%\Examples\MineSweep\ReadMe.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\MonthPlan\input_date,bdf"  %EXAMPLES_OUT_DIR%\Examples\MonthPlan\input_date.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\MonthPlan\monthplan,bdf"   %EXAMPLES_OUT_DIR%\Examples\MonthPlan\monthplan.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\MonthPlan\ReadMe,bdf"      %EXAMPLES_OUT_DIR%\Examples\MonthPlan\ReadMe.fwk
@rem
@copy /A "%EXAMPLES_SRC_DIR%\Examples\Mountains\Export,dfe"     %EXAMPLES_OUT_DIR%\Examples\Mountains\Export.csv
@rem FPro copy /B "%EXAMPLES_SRC_DIR%\Examples\Mountains\Mountain_d,c27"   %EXAMPLES_OUT_DIR%\Examples\Mountains\Mountain_d.c27
@rem FPro copy /A "%EXAMPLES_SRC_DIR%\Examples\Mountains\Mountain_f,bdf"   %EXAMPLES_OUT_DIR%\Examples\Mountains\Mountain_f.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\PipeDream\ReadMe,bdf"      %EXAMPLES_OUT_DIR%\Examples\PipeDream\ReadMe.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Pupils\Export,dfe"         %EXAMPLES_OUT_DIR%\Examples\Pupils\Export.dfe
@rem FPro copy /B "%EXAMPLES_SRC_DIR%\Examples\Pupils\Pupils_d,c27"        %EXAMPLES_OUT_DIR%\Examples\Pupils\Pupils_d.c27
@rem FPro copy /A "%EXAMPLES_SRC_DIR%\Examples\Pupils\Pupils_f,bdf"        %EXAMPLES_OUT_DIR%\Examples\Pupils\Pupils_f.fwk
@rem
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\0dp,c1d"            %EXAMPLES_OUT_DIR%\Examples\Styles\0dp.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\1dp,c1d"            %EXAMPLES_OUT_DIR%\Examples\Styles\1dp.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\ColStripe,c1d"      %EXAMPLES_OUT_DIR%\Examples\Styles\ColStripe.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\CurCell135,c1d"     %EXAMPLES_OUT_DIR%\Examples\Styles\CurCell135.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\CurCell200,c1d"     %EXAMPLES_OUT_DIR%\Examples\Styles\CurCell200.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\ProStyle,c1d"       %EXAMPLES_OUT_DIR%\Examples\Styles\ProStyle.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\Protection,c1d"     %EXAMPLES_OUT_DIR%\Examples\Styles\Protection.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\ReadMe,bdf"         %EXAMPLES_OUT_DIR%\Examples\Styles\ReadMe.fwk
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\RowStripe,c1d"      %EXAMPLES_OUT_DIR%\Examples\Styles\RowStripe.fwt
copy /A "%EXAMPLES_SRC_DIR%\Examples\Styles\ValueListT,c1d"     %EXAMPLES_OUT_DIR%\Examples\Styles\ValueListT.fwt
