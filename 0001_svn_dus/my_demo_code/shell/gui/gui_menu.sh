# !/bin/bash
echo "hello!"
#whiptail --title "<menu title>" --menu "<text to show>" <height> <width> <menu height> [ <tag> <item> ] . . .
OPTION=$(whiptail --title "Test Menu Dialog" --menu "Choose your option" 15 60 4 "1" "Grilled Spicy Sausage" "2" "Grilled Halloumi Cheese" "3" "Charcoaled Chicken Wings" "4" "Fried Aubergine" 3>&1 1>&2 2>&3)
status=$?
if [ $status = 0 ]; then
	echo "Your chosen option:" $OPTION
else
	echo "You chose Cancel."
fi 