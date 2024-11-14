/*
int main() {
    HANDLE hFile = CreateFile(L"vectors.db", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening file.\n";
        return 1;
    }

    BY_HANDLE_FILE_INFORMATION fileInfo;
    if (GetFileInformationByHandle(hFile, &fileInfo)) {
        std::cout << "File size: " << fileInfo.nFileSizeLow << " bytes\n";
        std::cout << "Cluster size: " << fileInfo.dwVolumeSerialNumber << " bytes\n";
    }
    else {
        std::cerr << "Error retrieving file information.\n";
    }

    CloseHandle(hFile);
    return 0;

}



int main() {
    readRawDisk("\\\\.\\PhysicalDrive0");  // Example path for the first physical drive
    return 0;
}
*/


/*
int main() {


       float Numbers[5] = { 0.034432,0.02343,0.234224, 0.832432 ,0.342234};
    size_t count = sizeof(Numbers) / sizeof(Numbers[0]);

 //   for (int i = 0; i < 5; i++) {
 //       write_floats_to_file("vectors.db", Numbers, count);
 //   };

    float* numbers_read;
    numbers_read = new float[count];
    read_floats_from_file("vectors.db", numbers_read, count);

    // Print the floating-point numbers read from the file
    printf("Floats read from file:\n");
    for (size_t i = 0; i < count; i++) {
        printf("%.3f\n", numbers_read[i]);
    }
   
    // Initialize the database with capacity for 10 entries
    InMemoryDatabase* db = init_database(10);
    if (db == NULL) {
        printf("Unable to initialize\n");
        return 1;
    }

    // Add data
    add_data(db, "243", "Hello");

    // Get and print the data
    char* value = get_data(db, "243");
    if (value != NULL) {
        printf("Key: 243, Value: %s\n", value);
    }
    else {
        printf("Key not found\n");
    }

    // Clean up the memory
    free(db->entries);
    free(db);

    return 0;
}
*/

includelib ucrt.lib
includelib legacy_stdio_definitions.lib

.DATA
    fmt_string_with_index byte "Character: %c at index: %d", 0  ; Format string for printf

EXTERN printf: PROC  ; Declare printf function

.CODE

GetCharAtAddress PROC
    test rcx, rcx
    jz end_loop            
    test rdx, rdx
    jz end_loop          


    xor r8, r8                 

read_loop:
    cmp r8, rdx                
    jge end_loop               
    
    mov al, byte ptr [rcx + r8] ; Load byte at (address + offset) into AL

    push rdx                  
    push rcx                  
    push r8                   

    movzx rcx, al             
    mov rdx, r8              
    lea r8, fmt_string_with_index 

   
    call printf                

    ; Restore registers after printf call
    pop r8                    
    pop rcx                  
    pop rdx                   

    ; Increment loop counter
    inc r8                     
    jmp read_loop           

end_loop:
    ret                         

GetCharAtAddress ENDP

END