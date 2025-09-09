# Software Requirements Specification - Home Financials

## Acronyms

| Acronym | Full Description                    |
----------|-------------------------------------|
| SRS     | Software Requirements Specification |

## References

| Shorthand | Full Title | Link |
|-----------|------------|------|


## Setting Up

**REQ-1**: The application must allow to set up groups. A group may be referred to as a *Home*. 

**REQ-2**: Each *Home* should allow inclusion of members. For the sake of continuty, a member of the group may be referred to as a *Family Member*.

**REQ-3**: A *Home* may only allow a maximum of 255 members (8-bit data restriction).

**REQ-4**: A *Home* may hold 0 members and no less (obviously!).

**REQ-5**: A *Home* may hold members of whole numbers only (even if one of the members is an infant). i.e. 

        Number of family members in Home A = 5 --> OK
        Number of family members in Home B = 2.5 --> NOT OK

## Learning To Read

**REQ-6**: The application must be able to read SB account statement of all relavant banks

**REQ-7**: The application must be able to read P&L statement of stock brokers

**REQ-8**: The application must be able to read P&L statement of Mutual Fund folios

**REQ-9**: The application must be able to read epf passbook.
