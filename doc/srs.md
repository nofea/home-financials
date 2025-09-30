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

**REQ-6**: The application must be able to read SB account statement of all relavant banks. The data to be read are,
1. Account Number
2. Name of the Account Holder(s)
3. IFSC info
4. Transaction details, i.e. Credit and Debit amount, source and destination of the transfer, Closing amount

**REQ-6.1**: The application must be able to read data from Canara bank account statement.

**REQ-6.2**: The application must be able to read data from HDFC bank account statement.

**REQ-6.3**: The application must be able to read data from SBI bank account statement.

**REQ-6.4**: The application must be able to read data from Axis bank account statement.

**REQ-6.5**: The application must be able to read data from PNB bank account statement.

**REQ-7**: The application must be able to read P&L statement of stock brokers.

**REQ-8**: The application must be able to read P&L statement of Mutual Fund folios.

**REQ-9**: The application must be able to read epf passbook.

## Vacancy

**REQ-10**: The data read in [Learning To Read](#learning-to-read) shoule be stored in a locally hosted SQLite database.

**REQ-11**: While the application is not running, the database must be compressed, and then decopressed only just before use.

**REQ-12**: Stored data older than 1 year should be archived by the database

## Hide Your Kids, Hide your Wifes

**REQ-13**: All locally stored data must be encrypted and then decryped when retrived.

**REQ-14**: All data in transit between the application and database must be encrypted with capability to decrypt on the point of receipt by the application or database.

## Show me the Money

**REQ-15**: Stored data must be displayed in human readable tabular format

**REQ-16**: The application must make provision to display stored data in a Terminal

**REQ-17**: The application must make provision to display stored data in a GUI with good UX.

**REQ-18**: The application must provide filters on the displayed data, i.e., Weekly/Monthly/Annually data, individual members in a Family, individual families, filter by bank, filter by account number
