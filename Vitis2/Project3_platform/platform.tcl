# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct D:\PortlandState\ECE_544\Labs\project_3\Vitis2\Project3_platform\platform.tcl
# 
# OR launch xsct and run below command.
# source D:\PortlandState\ECE_544\Labs\project_3\Vitis2\Project3_platform\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {Project3_platform}\
-hw {D:\PortlandState\ECE_544\Labs\project_2\nexysA7fpga.xsa}\
-proc {microblaze_0} -os {standalone} -fsbl-target {psu_cortexa53_0} -out {D:/PortlandState/ECE_544/Labs/project_3/Vitis2}

platform write
platform generate -domains 
platform active {Project3_platform}
platform generate
platform generate
