@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2014-2016 Stuart Swales

@rem Assumes N: is mapped to the root of your development tree i.e. the dir containing \fireworkz
@rem You can easily do this with a batch file dropped in your Startup folder
@rem     SUBST N: %USERPROFILE%\cs-dev
@rem or some appropriate equivalent for your dev environment.
@rem If not, you just have to navigate to this directory before executing the batch file
set SRC_DIR=N:\fireworkz\trunk\examples\Apps\Document\FzExamples
@rem
set OUT_DIR=%TEMP%\OUT_DIR
@rem
mkdir %OUT_DIR%
@rem
xcopy /T %SRC_DIR% %OUT_DIR%
@rem
copy /A "%SRC_DIR%\Chars,bdf"                %OUT_DIR%\Chars.fwk
copy /A "%SRC_DIR%\Commands,bdf"             %OUT_DIR%\Commands.fwk
copy /A "%SRC_DIR%\Keystrokes,bdf"           %OUT_DIR%\Keystrokes.fwk
copy /A "%SRC_DIR%\Numforms,bdf"             %OUT_DIR%\Numforms.fwk
copy /A "%SRC_DIR%\YearPlan14,bdf"           %OUT_DIR%\YearPlan14.fwk
@rem
copy /A "%SRC_DIR%\Charts\Bar,bdf"           %OUT_DIR%\Charts\Bar.fwk
copy /A "%SRC_DIR%\Charts\Bar_unempl,bdf"    %OUT_DIR%\Charts\Bar_unempl.fwk
copy /A "%SRC_DIR%\Charts\HistExam,bdf"      %OUT_DIR%\Charts\HistExam.fwk
copy /A "%SRC_DIR%\Charts\HistRead,bdf"      %OUT_DIR%\Charts\HistRead.fwk
copy /A "%SRC_DIR%\Charts\Line,bdf"          %OUT_DIR%\Charts\Line.fwk
copy /A "%SRC_DIR%\Charts\Penguin,bdf"       %OUT_DIR%\Charts\Penguin.fwk
copy /A "%SRC_DIR%\Charts\Pie,bdf"           %OUT_DIR%\Charts\Pie.fwk
copy /A "%SRC_DIR%\Charts\XY,bdf"            %OUT_DIR%\Charts\XY.fwk
@rem
copy /A "%SRC_DIR%\Custom\Custom,bdf"        %OUT_DIR%\Custom\Custom.fwk
copy /A "%SRC_DIR%\Custom\c_Fri13,bdf"       %OUT_DIR%\Custom\c_Fri13.fwk
copy /A "%SRC_DIR%\Custom\c_tangen,bdf"      %OUT_DIR%\Custom\c_tangen.fwk
copy /A "%SRC_DIR%\Custom\c_total,bdf"       %OUT_DIR%\Custom\c_total.fwk
copy /A "%SRC_DIR%\Custom\Fri13,bdf"         %OUT_DIR%\Custom\Fri13.fwk
copy /A "%SRC_DIR%\Custom\Fri13Read,bdf"     %OUT_DIR%\Custom\Fri13Read.fwk
copy /A "%SRC_DIR%\Custom\tangent,bdf"       %OUT_DIR%\Custom\tangent.fwk
copy /A "%SRC_DIR%\Custom\total,bdf"         %OUT_DIR%\Custom\total.fwk
@rem
                                       rd /q %OUT_DIR%\DB_Lookup                            
@rem
copy /A "%SRC_DIR%\Draft\draft_db,c1d"       %OUT_DIR%\Draft\draft_db.fwt
copy /A "%SRC_DIR%\Draft\draft_rz,c1d"       %OUT_DIR%\Draft\draft_rz.fwt
copy /A "%SRC_DIR%\Draft\draft_wz,c1d"       %OUT_DIR%\Draft\draft_wz.fwt
copy /A "%SRC_DIR%\Draft\ReadMe,bdf"         %OUT_DIR%\Draft\ReadMe.fwk
copy /A "%SRC_DIR%\Draft\test,bdf"           %OUT_DIR%\Draft\test.fwk
@rem
copy /A "%SRC_DIR%\Functions\complex,bdf"    %OUT_DIR%\Functions\complex.fwk
copy /A "%SRC_DIR%\Functions\database,bdf"   %OUT_DIR%\Functions\database.fwk
copy /A "%SRC_DIR%\Functions\date,bdf"       %OUT_DIR%\Functions\date.fwk
copy /A "%SRC_DIR%\Functions\finance,bdf"    %OUT_DIR%\Functions\finance.fwk
copy /A "%SRC_DIR%\Functions\linest,bdf"     %OUT_DIR%\Functions\linest.fwk
copy /A "%SRC_DIR%\Functions\linest_m,bdf"   %OUT_DIR%\Functions\linest_m.fwk
copy /A "%SRC_DIR%\Functions\listcount,bdf"  %OUT_DIR%\Functions\listcount.fwk
copy /A "%SRC_DIR%\Functions\lookup,bdf"     %OUT_DIR%\Functions\lookup.fwk
copy /A "%SRC_DIR%\Functions\maths,bdf"      %OUT_DIR%\Functions\maths.fwk
copy /A "%SRC_DIR%\Functions\matrix,bdf"     %OUT_DIR%\Functions\matrix.fwk
copy /A "%SRC_DIR%\Functions\misc,bdf"       %OUT_DIR%\Functions\misc.fwk
copy /A "%SRC_DIR%\Functions\spearman,bdf"   %OUT_DIR%\Functions\spearman.fwk
copy /A "%SRC_DIR%\Functions\statistic,bdf"  %OUT_DIR%\Functions\statistic.fwk
copy /A "%SRC_DIR%\Functions\string,bdf"     %OUT_DIR%\Functions\string.fwk
copy /A "%SRC_DIR%\Functions\trig,bdf"       %OUT_DIR%\Functions\trig.fwk

@rem
copy /A "%SRC_DIR%\Labels\csv_file,dfe"      %OUT_DIR%\Labels\csv_file.csv
copy /A "%SRC_DIR%\Labels\L7161,bdf"         %OUT_DIR%\Labels\L7161.fwk
copy /A "%SRC_DIR%\Labels\Label3x8,bdf"      %OUT_DIR%\Labels\Label3x8.fwk
copy /A "%SRC_DIR%\Labels\ReadMe,bdf"        %OUT_DIR%\Labels\ReadMe.fwk
@rem
copy /A "%SRC_DIR%\MineSweep\!PlayMe,bdf"    %OUT_DIR%\MineSweep\!PlayMe.fwk
copy /A "%SRC_DIR%\MineSweep\Board20,bdf"    %OUT_DIR%\MineSweep\Board20.fwk
copy /A "%SRC_DIR%\MineSweep\c_game,bdf"     %OUT_DIR%\MineSweep\c_game.fwk
copy /A "%SRC_DIR%\MineSweep\ReadMe,bdf"     %OUT_DIR%\MineSweep\ReadMe.fwk
@rem
copy /A "%SRC_DIR%\MonthPlan\input_date,bdf" %OUT_DIR%\MonthPlan\input_date.fwk
copy /A "%SRC_DIR%\MonthPlan\monthplan,bdf"  %OUT_DIR%\MonthPlan\monthplan.fwk
copy /A "%SRC_DIR%\MonthPlan\ReadMe,bdf"     %OUT_DIR%\MonthPlan\ReadMe.fwk
@rem
@rem FPro copy /B "%SRC_DIR%\Mountains\Mountain_d,c27"   %OUT_DIR%\Mountains\Mountain_d.c27
@rem FPro copy /A "%SRC_DIR%\Mountains\Mountain_f,bdf"   %OUT_DIR%\Mountains\Mountain_f.fwk
                                       rd /q %OUT_DIR%\Mountains
@rem
copy /A "%SRC_DIR%\New\AutoFormat,bdf"       %OUT_DIR%\New\AutoFormat.fwk
copy /A "%SRC_DIR%\New\ButtonBar,bdf"        %OUT_DIR%\New\ButtonBar.fwk
copy /A "%SRC_DIR%\New\InCellEdit,bdf"       %OUT_DIR%\New\InCellEdit.fwk
copy /A "%SRC_DIR%\New\NumStyles,bdf"        %OUT_DIR%\New\NumStyles.fwk
copy /A "%SRC_DIR%\New\Stripes,bdf"          %OUT_DIR%\New\Stripes.fwk
copy /A "%SRC_DIR%\New\ValueLists,bdf"       %OUT_DIR%\New\ValueLists.fwk
@rem
copy /A "%SRC_DIR%\PipeDream\ReadMe,bdf"     %OUT_DIR%\PipeDream\ReadMe.fwk
@rem
@rem FPro copy /B "%SRC_DIR%\Pupils\Pupils_d,c27"        %OUT_DIR%\Pupils\Pupils_d.c27
@rem FPro copy /A "%SRC_DIR%\Pupils\Pupils_f,bdf"        %OUT_DIR%\Pupils\Pupils_f.fwk
                                       rd /q %OUT_DIR%\Pupils                               
@rem
copy /A "%SRC_DIR%\Styles\0dp,c1d"           %OUT_DIR%\Styles\0dp.fwt
copy /A "%SRC_DIR%\Styles\1dp,c1d"           %OUT_DIR%\Styles\1dp.fwt
copy /A "%SRC_DIR%\Styles\ColStripe,c1d"     %OUT_DIR%\Styles\ColStripe.fwt
copy /A "%SRC_DIR%\Styles\CurCell135,c1d"    %OUT_DIR%\Styles\CurCell135.fwt
copy /A "%SRC_DIR%\Styles\CurCell200,c1d"    %OUT_DIR%\Styles\CurCell200.fwt
copy /A "%SRC_DIR%\Styles\ProStyle,c1d"      %OUT_DIR%\Styles\ProStyle.fwt
copy /A "%SRC_DIR%\Styles\Protection,c1d"    %OUT_DIR%\Styles\Protection.fwt
copy /A "%SRC_DIR%\Styles\ReadMe,bdf"        %OUT_DIR%\Styles\ReadMe.fwk
copy /A "%SRC_DIR%\Styles\RowStripe,c1d"     %OUT_DIR%\Styles\RowStripe.fwt
copy /A "%SRC_DIR%\Styles\ValueListT,c1d"    %OUT_DIR%\Styles\ValueListT.fwt
@rem
copy /A "%SRC_DIR%\Tutorial\Accounts,bdf"    %OUT_DIR%\Tutorial\Accounts.fwk
copy /A "%SRC_DIR%\Tutorial\Exams,bdf"       %OUT_DIR%\Tutorial\Exams.fwk
copy /B "%SRC_DIR%\Tutorial\firepic,ff9"     %OUT_DIR%\Tutorial\firepic.ff9
copy /A "%SRC_DIR%\Tutorial\Form,bdf"        %OUT_DIR%\Tutorial\Form.fwk
@rem FPro copy /A "%SRC_DIR%\Tutorial\names,bdf"       %OUT_DIR%\Tutorial\names.fwk
@rem FPro copy /B "%SRC_DIR%\Tutorial\names_b,c27"     %OUT_DIR%\Tutorial\names_b.c27
copy /A "%SRC_DIR%\Tutorial\Newsletter,bdf"  %OUT_DIR%\Tutorial\Newsletter.fwk
copy /A "%SRC_DIR%\Tutorial\Report,bdf"      %OUT_DIR%\Tutorial\Report.fwk
copy /A "%SRC_DIR%\Tutorial\saleslet2,bdf"   %OUT_DIR%\Tutorial\saleslet2.fwk
copy /A "%SRC_DIR%\Tutorial\Theatre,bdf"     %OUT_DIR%\Tutorial\Theatre.fwk
@pause
