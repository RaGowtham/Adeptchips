#include "include.h"

void export_unexport(char *alias)
{
    int option, pin_no;
    char *command = calloc(10,sizeof(char));

redo:	printf("Choose Your Choice : \n\t1) For Exporting\n\t2) For Unexporting\nEnter Your Choice : ");
	scanf("%d",&option);
	switch(option)
	{
	    case 1:
pin:			printf("Enter Pin no : ");
			scanf("%d",&pin_no);
			if(pin_no < 0 || pin_no > 100)
			{
			    printf("Enter Correct value\n");
			    goto pin;
			}
			sprintf(command,"%s.%03dXX",alias,pin_no);
			break;
	    case 2:
pin1:			printf("Enter Pin no : ");
			scanf("%d",&pin_no);
			if(pin_no < 0 || pin_no > 100)
			{
			    printf("Enter Correct value\n");
			    goto pin1;
			}
			sprintf(command,"%s.%03dXX",alias,-1 * pin_no);
			break;
	    default:
			printf("Enter Correct Option\n");
			goto redo;
	}
	send("Control",command);
}

void pin_configuration(char *alias)
{
    int option, pin_no, direction, value;
    char *command = calloc(10,sizeof(char));

redo:	printf("Choose Your Choice : \n\t1) For Direction\n\t2) For Value\n\t3) For Both\nEnter Your Choice : ");
	scanf("%d",&option);
	switch(option)
	{
	    case 1:
pin:			printf("Enter Pin no : ");
			scanf("%d",&pin_no);
			if(pin_no < 0 || pin_no > 100)
			{
			    printf("Enter Correct Pin No\n");
			    goto pin;
			}
			printf("Enter Direction\n\tPress \"0\" For Input\n\tPress \"1\" For Output\nEnter Choice: ");
			scanf("%d",&direction);
			if(direction == 1)
			{
			    sprintf(command,"%s.%03d%cX",alias,'I',pin_no);
			}
			else if(direction == 0)
			{
			    sprintf(command,"%s.%03d%cX",alias,'O',pin_no);
			}
			break;
	    case 3:
pin1:			printf("Enter Pin no : ");
			scanf("%d",&pin_no);
			if(pin_no < 0 || pin_no > 100)
			{
			    printf("Enter Correct Pin No\n");
			    goto pin1;
			}
value1:			printf("Enter Value\n\tPress \"0\" For Low\n\tPress \"1\" For High\nEnter Choice: ");
			scanf("%d",&value);
			printf("Enter Direction\n\tPress \"0\" For Input\n\tPress \"1\" For Output\nEnter Choice: ");
			scanf("%d",&direction);
			if(value == 1)
			{
			    if(direction == 1)
			    {
				sprintf(command,"%s.%03d%c%d",alias,pin_no,'I',1);
			    }
			    else if(direction == 0)
			    {
				sprintf(command,"%s.%03d%c%d",alias,pin_no,'O',1);
			    }
			    else
			    {
				printf("Enter Proper Option\n");
				goto  value1;
			    }
			}
			else if(value == 0)
			{
			    if(direction == 1)
			    {
				sprintf(command,"%s.%03d%c%d",alias,pin_no,'I',0);
			    }
			    else if(direction == 0)
			    {
				sprintf(command,"%s.%03d%c%d",alias,pin_no,'O',0);
			    }
			    else
			    {
				printf("Enter Proper Option\n");
				goto  value1;
			    }
			}
			else
			{
			    printf("Enter Correct Direction\n");
			    goto pin1;
			}
			break;
	    case 2:
pin2:			printf("Enter Pin no : ");
			scanf("%d",&pin_no);
			if(pin_no < 0 || pin_no > 100)
			{
			    printf("Enter Correct Pin No\n");
			    goto pin2;
			}
			printf("Enter Value\n\tPress \"0\" For Low\n\tPress \"1\" For High\nEnter Choice: ");
			scanf("%d",&value);
			if(value == 1)
			{
			    sprintf(command,"%s.%03dX%d",alias,pin_no,1);
			}
			else if(value == 0)
			{
			    sprintf(command,"%s.%03dX%d",alias,pin_no,0);
			}
			else
			{
			    printf("Enter Correct value\n");
			    goto pin2;
			}
			break;
	    default:
			printf("Enter Correct Option\n");
			goto redo;
	}
	send("Control",command);
}

int control_send(char *alias)
{
    int option;
redo:		printf("Choose Your Choice : \n\t1) For Exporting or Unexporting\n\t2) For Pin Configuration\nEnter Your Choice : ");
		scanf("%d",&option);
		switch(option)
		{
		    case 1:
			export_unexport(alias);
			break;
		    case 2:
			pin_configuration(alias);
			break;
		    default:
			printf("Enter Correct Option\n");
			goto redo;
		}
}

int control_recv(char *alias)
{
    int i;
    while(1)
    {
	char *info;
	while(1)
	{
	    info = get("Control");
	    for(i = 0;info[i] != 0; i++)
	    {
		if(info[i] == '.')
		{
		    info[i] = 0;
		    break;
		}
	    }
	    if((strcmp(info,alias)) == 0)
	    {
		break;
	    }
	}
	int pin_no = atoi(&info[i + 1]);
	char direction = info[i + 4];
	int value = atoi(&info[i + 5]);

	if(pin_no >= 0 && pin_no < 100)
	{
	    char *command = calloc(100,sizeof(char));
	    sprintf(command,"echo %d > /sys/class/gpio/export",pin_no);
	    system(command);
	    free(command);
	}
	else if(pin_no < 0 && pin_no > -100)
	{
	    char *command = calloc(100,sizeof(char));
	    sprintf(command,"echo %d > /sys/class/gpio/unexport",-1 * pin_no);
	    system(command);
	    free(command);
	    continue;
	}
	if(info[i + 4] == 'i' ||  info[i + 4] == 'I')
	{
	    char *command = calloc(100,sizeof(char));
	    sprintf(command,"echo %s > /sys/class/gpio/gpio%d/direction","in",pin_no);
	    system(command);
	    free(command);
	}
	else if(info[i + 4] == 'o' ||  info[i + 4] == 'O')
	{
	    char *command = calloc(100,sizeof(char));
	    sprintf(command,"echo %s > /sys/class/gpio/gpio%d/direction","out",pin_no);
	    system(command);
	    free(command);
	}
	if(value > 0 && value < 10)
	{
	    char *command = calloc(100,sizeof(char));
	    sprintf(command,"echo %d > /sys/class/gpio/gpio%d/value",value,pin_no);
	    system(command);
	    free(command);
	}
	else if( value == 0)
	{
	    char *command = calloc(100,sizeof(char));
	    sprintf(command,"echo %d > /sys/class/gpio/gpio%d/value",value,pin_no);
	    system(command);
	    free(command);
	}
    }
}
