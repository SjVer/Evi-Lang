#ifndef EVI_IR_OBJECT_H
#define EVI_IR_OBJECT_H


/*
Example of the process:

    expression: 2 + 3 * 4

    2:	push(2)         	    [2]

    +:	do

        3:	push(3)    		    [2, 3]

        *: do
        
            4:  push(4)         [2, 3, 4]

            done

        rhs = pop(4)          [2, 3]
        lhs = pop(3)          [2]
        tmp1 = mult(lhs, rhs)
        push(*tmp1)            [2, *tmp1]

        done

    rhs = pop(*tmp1)           [2]
    lhs = pop(2)              []
    tmp2 = add(lhs, rhs)      
    push(*tmp2)                [*tmp2]

    done
*/

#endif // !EVI_IR_OBJECT_H