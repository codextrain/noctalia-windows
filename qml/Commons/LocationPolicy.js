function manual(current, address) {
    if(typeof address!=="string"||!address.trim()) return null;
    var next=Object.assign({},current||{});
    next.address=address.trim();next.auto_locate=false;
    return next;
}
